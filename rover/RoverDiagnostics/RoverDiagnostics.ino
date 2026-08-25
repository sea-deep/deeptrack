#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <DHT.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_VL53L0X.h>
#include <ESP32Servo.h>
#include <esp_arduino_version.h>
#include <math.h>

namespace Pin {
constexpr uint8_t DHT_DATA = 23, GAS = 36, WATER = 39;
constexpr uint8_t TRIG = 19, ECHO = 18;
constexpr uint8_t SDA = 21, SCL = 22;
constexpr uint8_t LEFT_ENCODER = 34, RIGHT_ENCODER = 35;
constexpr uint8_t SERVO = 13, BUZZER = 4, RED = 26, GREEN = 27;
constexpr uint8_t LEFT_PWM = 25, LEFT_IN1 = 14, LEFT_IN2 = 16;
constexpr uint8_t RIGHT_PWM = 17, RIGHT_IN1 = 33, RIGHT_IN2 = 32;
}

constexpr uint8_t DHT_MODEL = DHT22;  // Change to DHT11 only for a DHT11.
constexpr uint8_t LEFT_PWM_CHANNEL = 4, RIGHT_PWM_CHANNEL = 5;
constexpr uint32_t PWM_FREQUENCY = 20000;
constexpr uint8_t PWM_RESOLUTION = 8;
constexpr int SERVO_CENTER = 90;
constexpr int SCAN_ANGLES[] = {25, 55, 90, 125, 155};
constexpr uint8_t SCAN_COUNT = sizeof(SCAN_ANGLES) / sizeof(SCAN_ANGLES[0]);
constexpr uint32_t SERVO_SETTLE_MS = 180;
constexpr uint32_t REVERSE_MS = 280;
constexpr uint32_t MANUAL_RUN_MS = 5000;
constexpr uint32_t AUTORUN_DELAY_MS = 5000;
constexpr float STOP_DISTANCE_CM = 22.0f;
constexpr float EMERGENCY_DISTANCE_CM = 10.0f;
constexpr float SLOW_DISTANCE_CM = 45.0f;
constexpr float CLEAR_PATH_CM = 42.0f;
constexpr float TILT_RECOVERY_DEGREES = 42.0f;
constexpr float TILT_RESET_DEGREES = 34.0f;
constexpr float SEVERE_TILT_DEGREES = 65.0f;
constexpr uint32_t TILT_CONFIRM_MS = 550;
constexpr uint32_t SEVERE_TILT_CONFIRM_MS = 180;
constexpr uint32_t TILT_RECOVERY_COOLDOWN_MS = 2500;
constexpr float TILT_FILTER_ALPHA = 0.16f;

DHT dht(Pin::DHT_DATA, DHT_MODEL);
Adafruit_MPU6050 imu;
Adafruit_VL53L0X tof;
Servo scanner;
Preferences preferences;

volatile uint32_t leftTicks = 0, rightTicks = 0;
bool hasImu = false, hasTof = false, hasServo = false;
// The chassis is mounted with its original front facing backward.
// Both motor banks therefore default to reversed electrical direction.
bool leftInverted = true, rightInverted = true;
bool lowAngleIsLeft = false;
bool autoEnabled = false, pendingAutorun = false;
bool encoderHasMoved = false, gasAlarm = false;
bool buzzerActiveLow = false;
uint8_t cruisePwm = 210;
int waterThreshold = 2600, gasThreshold = 3400;
int lastGas = 0, lastWater = 0;
float temperatureC = NAN, humidityPercent = NAN;
float sonarCm = NAN, centerTofCm = NAN, frontCm = NAN;
float initialPitch = 0, initialRoll = 0, rawTilt = 0, currentTilt = 0;
float gyroBiasZ = 0, accumulatedTurnDegrees = 0;
float scanDistances[SCAN_COUNT] = {};
uint32_t bootMs = 0, lastFrontMs = 0, lastEnvironmentMs = 0;
uint32_t lastImuMs = 0, lastEncoderMs = 0, lastTelemetryMs = 0;
uint32_t stateStartMs = 0, manualUntilMs = 0, lastTurnSampleMs = 0;
uint32_t tiltAboveSinceMs = 0, tiltRecoveryCooldownUntilMs = 0;
uint32_t previousLeftTicks = 0, previousRightTicks = 0;
uint8_t missingFrontSamples = 0, stationaryWindows = 0;
uint8_t obstacleSamples = 0;
uint8_t scanIndex = 0, blockedAttempts = 0;
bool tiltFilterReady = false;
bool turnLeft = true;
float targetTurnDegrees = 60.0f;
int leftCorrection = 0, rightCorrection = 0;
String serialLine;

enum class State : uint8_t { STOPPED, CRUISE, REVERSING, SCANNING, TURNING, HALTED };
State state = State::STOPPED;

void IRAM_ATTR onLeftEncoder() { leftTicks++; }
void IRAM_ATTR onRightEncoder() { rightTicks++; }

const char *stateName() {
  switch (state) {
    case State::STOPPED: return "STOPPED";
    case State::CRUISE: return "CRUISE";
    case State::REVERSING: return "REVERSING";
    case State::SCANNING: return "SCANNING";
    case State::TURNING: return "TURNING";
    case State::HALTED: return "HALTED";
  }
  return "UNKNOWN";
}

void setBuzzer(bool enabled) {
  digitalWrite(Pin::BUZZER, enabled ^ buzzerActiveLow ? HIGH : LOW);
}

void writePwm(uint8_t pin, uint8_t channel, uint8_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)channel;
  ledcWrite(pin, duty);
#else
  (void)pin;
  ledcWrite(channel, duty);
#endif
}

bool attachMotorPwm(uint8_t pin, uint8_t channel) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  return ledcAttachChannel(pin, PWM_FREQUENCY, PWM_RESOLUTION, channel);
#else
  ledcSetup(channel, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(pin, channel);
  return true;
#endif
}

void setMotor(uint8_t in1, uint8_t in2, uint8_t pwmPin, uint8_t channel,
              int power, bool inverted) {
  power = constrain(power, -255, 255);
  if (inverted) power = -power;
  if (power == 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    writePwm(pwmPin, channel, 0);
    return;
  }
  digitalWrite(in1, power > 0 ? HIGH : LOW);
  digitalWrite(in2, power > 0 ? LOW : HIGH);
  writePwm(pwmPin, channel, abs(power));
}

void drive(int left, int right) {
  setMotor(Pin::LEFT_IN1, Pin::LEFT_IN2, Pin::LEFT_PWM, LEFT_PWM_CHANNEL,
           left, leftInverted);
  setMotor(Pin::RIGHT_IN1, Pin::RIGHT_IN2, Pin::RIGHT_PWM, RIGHT_PWM_CHANNEL,
           right, rightInverted);
}

void stopMotors() { drive(0, 0); }

void setIndicators() {
  bool hazard = gasAlarm || state == State::HALTED;
  digitalWrite(Pin::RED, hazard ? HIGH : LOW);
  digitalWrite(Pin::GREEN, autoEnabled && !hazard ? HIGH : LOW);
}

float readSonar() {
  digitalWrite(Pin::TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(Pin::TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(Pin::TRIG, LOW);
  unsigned long pulse = pulseIn(Pin::ECHO, HIGH, 22000);
  if (pulse == 0) return NAN;
  float distance = pulse * 0.0343f / 2.0f;
  return distance >= 2.0f && distance <= 350.0f ? distance : NAN;
}

float readTof() {
  if (!hasTof) return NAN;
  VL53L0X_RangingMeasurementData_t measurement;
  tof.rangingTest(&measurement, false);
  if (measurement.RangeStatus == 4 || measurement.RangeMilliMeter < 30 ||
      measurement.RangeMilliMeter > 1600) {
    return NAN;
  }
  return measurement.RangeMilliMeter / 10.0f;
}

float readTofMedian() {
  float samples[3];
  uint8_t count = 0;
  for (uint8_t i = 0; i < 3; i++) {
    float value = readTof();
    if (!isnan(value)) samples[count++] = value;
  }
  if (count == 0) return NAN;
  if (count == 2) return (samples[0] + samples[1]) / 2.0f;
  if (count == 1) return samples[0];
  if (samples[0] > samples[1]) { float t = samples[0]; samples[0] = samples[1]; samples[1] = t; }
  if (samples[1] > samples[2]) { float t = samples[1]; samples[1] = samples[2]; samples[2] = t; }
  if (samples[0] > samples[1]) { float t = samples[0]; samples[0] = samples[1]; samples[1] = t; }
  return samples[1];
}

void updateFrontDistance() {
  sonarCm = readSonar();
  centerTofCm = hasTof && hasServo && abs(scanner.read() - SERVO_CENTER) <= 2
                    ? readTof()
                    : NAN;
  if (!isnan(sonarCm) && !isnan(centerTofCm)) frontCm = min(sonarCm, centerTofCm);
  else if (!isnan(sonarCm)) frontCm = sonarCm;
  else frontCm = centerTofCm;

  if (isnan(frontCm)) missingFrontSamples++;
  else missingFrontSamples = 0;
}

void updateEnvironment(bool includeDht) {
  lastGas = analogRead(Pin::GAS);
  lastWater = analogRead(Pin::WATER);
  gasAlarm = lastGas >= gasThreshold;
  if (includeDht) {
    temperatureC = dht.readTemperature();
    humidityPercent = dht.readHumidity();
  }
  setIndicators();
}

void calibrateImu() {
  if (!hasImu) return;
  float pitchSum = 0, rollSum = 0, gyroSum = 0;
  for (uint8_t i = 0; i < 20; i++) {
    sensors_event_t acceleration, gyro, temp;
    imu.getEvent(&acceleration, &gyro, &temp);
    pitchSum += atan2f(-acceleration.acceleration.x,
                       sqrtf(acceleration.acceleration.y * acceleration.acceleration.y +
                             acceleration.acceleration.z * acceleration.acceleration.z)) * 180.0f / PI;
    rollSum += atan2f(acceleration.acceleration.y, acceleration.acceleration.z) * 180.0f / PI;
    gyroSum += gyro.gyro.z;
    delay(15);
  }
  initialPitch = pitchSum / 20.0f;
  initialRoll = rollSum / 20.0f;
  gyroBiasZ = gyroSum / 20.0f;
  rawTilt = 0;
  currentTilt = 0;
  tiltFilterReady = false;
  tiltAboveSinceMs = 0;
}

void updateImu() {
  if (!hasImu) return;
  sensors_event_t acceleration, gyro, temp;
  imu.getEvent(&acceleration, &gyro, &temp);
  float pitch = atan2f(-acceleration.acceleration.x,
                       sqrtf(acceleration.acceleration.y * acceleration.acceleration.y +
                             acceleration.acceleration.z * acceleration.acceleration.z)) * 180.0f / PI;
  float roll = atan2f(acceleration.acceleration.y, acceleration.acceleration.z) * 180.0f / PI;
  float pitchDelta = pitch - initialPitch;
  float rollDelta = roll - initialRoll;
  while (rollDelta > 180.0f) rollDelta -= 360.0f;
  while (rollDelta < -180.0f) rollDelta += 360.0f;
  rawTilt = sqrtf(pitchDelta * pitchDelta + rollDelta * rollDelta);
  if (!tiltFilterReady) {
    currentTilt = rawTilt;
    tiltFilterReady = true;
  } else {
    currentTilt += TILT_FILTER_ALPHA * (rawTilt - currentTilt);
  }
  if (state == State::TURNING) {
    uint32_t now = millis();
    float elapsedSeconds = (now - lastTurnSampleMs) / 1000.0f;
    float yawRate = fabsf(gyro.gyro.z - gyroBiasZ);
    if (yawRate > 0.035f && elapsedSeconds < 0.3f) {
      accumulatedTurnDegrees += yawRate * elapsedSeconds * 180.0f / PI;
    }
    lastTurnSampleMs = now;
  }
}

void emergencyStop(const char *reason) {
  stopMotors();
  autoEnabled = false;
  pendingAutorun = false;
  state = State::HALTED;
  setBuzzer(true);
  setIndicators();
  Serial.printf("HALTED: %s\n", reason);
}

void stopEverything() {
  stopMotors();
  autoEnabled = false;
  pendingAutorun = false;
  manualUntilMs = 0;
  state = State::STOPPED;
  setBuzzer(false);
  if (hasServo) scanner.write(SERVO_CENTER);
  setIndicators();
}

void beginAvoidance(const char *reason) {
  stopMotors();
  obstacleSamples = 0;
  blockedAttempts++;
  Serial.printf("AVOID: %s | front=%.1f cm | attempt=%u\n",
                reason, frontCm, blockedAttempts);
  drive(-210, -210);
  state = State::REVERSING;
  stateStartMs = millis();
}

void startTurn(bool goLeft, float degrees) {
  turnLeft = goLeft;
  targetTurnDegrees = degrees;
  accumulatedTurnDegrees = 0;
  lastTurnSampleMs = millis();
  if (hasServo) scanner.write(SERVO_CENTER);
  drive(goLeft ? -205 : 205, goLeft ? 205 : -205);
  state = State::TURNING;
  stateStartMs = millis();
  Serial.printf("TURN: %s, target=%.0f degrees\n", goLeft ? "LEFT" : "RIGHT", degrees);
}

float scanValue(uint8_t index) {
  return isnan(scanDistances[index]) ? 0.0f : scanDistances[index];
}

float corridorScore(float first, float second) {
  if (first <= 0 && second <= 0) return 0;
  if (first <= 0) return second * 0.55f;
  if (second <= 0) return first * 0.55f;
  return min(first, second) * 0.70f + max(first, second) * 0.30f;
}

void selectEscapeDirection() {
  float lowOuter = scanValue(0);
  float lowInner = scanValue(1);
  float center = scanValue(2);
  float highInner = scanValue(3);
  float highOuter = scanValue(4);
  float lowScore = corridorScore(lowOuter, lowInner);
  float highScore = corridorScore(highInner, highOuter);
  float innerClearance = min(lowInner, highInner);
  float centerScore = center > 0 && innerClearance > 0
                          ? center * 0.70f + innerClearance * 0.30f
                          : center * 0.55f;
  float bestSideScore = max(lowScore, highScore);

  Serial.printf("SCAN: 25=%.1f 55=%.1f 90=%.1f 125=%.1f 155=%.1f cm\n",
                scanDistances[0], scanDistances[1], scanDistances[2],
                scanDistances[3], scanDistances[4]);

  bool centerIsSafe = center >= CLEAR_PATH_CM &&
                      lowInner > STOP_DISTANCE_CM + 5.0f &&
                      highInner > STOP_DISTANCE_CM + 5.0f &&
                      centerScore >= bestSideScore - 8.0f;
  if (centerIsSafe) {
    scanner.write(SERVO_CENTER);
    state = State::CRUISE;
    stateStartMs = millis();
    lastFrontMs = 0;
    obstacleSamples = 0;
    Serial.printf("PATH: STRAIGHT, clearance score=%.1f cm\n", centerScore);
    return;
  }

  bool chooseLow = lowScore > highScore;
  bool chooseLeft = chooseLow ? lowAngleIsLeft : !lowAngleIsLeft;
  float bestClearance = max(lowScore, highScore);
  float selectedOuter = chooseLow ? lowOuter : highOuter;
  float selectedInner = chooseLow ? lowInner : highInner;
  float degrees = bestClearance < STOP_DISTANCE_CM + 5.0f ? 105.0f :
                  selectedOuter > selectedInner + 15.0f ? 80.0f : 55.0f;
  if (bestClearance < 5.0f) chooseLeft = blockedAttempts % 2 != 0;
  Serial.printf("PATH: %s, score=%.1f cm, turn=%.0f degrees\n",
                chooseLeft ? "LEFT" : "RIGHT", bestClearance, degrees);
  startTurn(chooseLeft, degrees);
}

void updateEncoderBalance() {
  uint32_t left = leftTicks, right = rightTicks;
  uint32_t deltaLeft = left - previousLeftTicks;
  uint32_t deltaRight = right - previousRightTicks;
  previousLeftTicks = left;
  previousRightTicks = right;

  if (deltaLeft > 0 || deltaRight > 0) encoderHasMoved = true;
  if (!encoderHasMoved || state != State::CRUISE) return;

  if (deltaLeft == 0 && deltaRight == 0) {
    if (++stationaryWindows >= 6) {
      stationaryWindows = 0;
      beginAvoidance("encoder stall");
    }
    return;
  }
  stationaryWindows = 0;

  int difference = (int)deltaLeft - (int)deltaRight;
  if (abs(difference) >= 2) {
    int adjustment = constrain(difference * 3, -24, 24);
    leftCorrection = constrain(leftCorrection - adjustment, -35, 35);
    rightCorrection = constrain(rightCorrection + adjustment, -35, 35);
  }
}

void startAutonomy() {
  stopMotors();
  if (!hasTof) {
    Serial.println("NOTICE: VL53L0X unavailable; using ultrasonic-only alternating turns.");
  } else if (!hasServo) {
    Serial.println("NOTICE: servo unavailable; directional scanning is disabled.");
  }
  if (hasServo) scanner.write(SERVO_CENTER);
  missingFrontSamples = 0;
  obstacleSamples = 0;
  stationaryWindows = 0;
  blockedAttempts = 0;
  tiltAboveSinceMs = 0;
  tiltRecoveryCooldownUntilMs = 0;
  leftCorrection = rightCorrection = 0;
  autoEnabled = true;
  pendingAutorun = false;
  setBuzzer(false);
  state = State::CRUISE;
  stateStartMs = millis();
  lastFrontMs = 0;
  setIndicators();
  Serial.println("AUTONOMOUS MODE ENABLED");
}

void updateTiltRecovery(uint32_t now) {
  if (!hasImu || now - lastImuMs < 35) return;
  lastImuMs = now;
  updateImu();

  if (currentTilt <= TILT_RESET_DEGREES) {
    tiltAboveSinceMs = 0;
    return;
  }
  if (currentTilt < TILT_RECOVERY_DEGREES) return;
  if (tiltAboveSinceMs == 0) tiltAboveSinceMs = now;

  uint32_t requiredMs = currentTilt >= SEVERE_TILT_DEGREES
                            ? SEVERE_TILT_CONFIRM_MS
                            : TILT_CONFIRM_MS;
  bool cooldownFinished = (int32_t)(now - tiltRecoveryCooldownUntilMs) >= 0;
  if (state == State::CRUISE && cooldownFinished &&
      now - tiltAboveSinceMs >= requiredMs) {
    Serial.printf("TILT RECOVERY: filtered=%.1f raw=%.1f degrees\n",
                  currentTilt, rawTilt);
    tiltAboveSinceMs = 0;
    tiltRecoveryCooldownUntilMs = now + TILT_RECOVERY_COOLDOWN_MS;
    beginAvoidance("sustained tilt; backing away and rescanning");
  }
}

void updateAutonomy() {
  if (!autoEnabled) return;
  uint32_t now = millis();

  if (now - lastEnvironmentMs >= 250) {
    lastEnvironmentMs = now;
    updateEnvironment(false);
    if (lastWater >= waterThreshold) {
      emergencyStop("water threshold exceeded");
      return;
    }
  }

  updateTiltRecovery(now);

  if (state == State::CRUISE) {
    if (now - lastFrontMs >= 85) {
      lastFrontMs = now;
      updateFrontDistance();
      if (missingFrontSamples >= 5) {
        emergencyStop("both forward distance sensors unavailable");
        return;
      }
      if (!isnan(frontCm) && frontCm <= EMERGENCY_DISTANCE_CM) {
        beginAvoidance("very close obstacle");
        return;
      }
      if (!isnan(frontCm) && frontCm <= STOP_DISTANCE_CM) {
        if (++obstacleSamples >= 2) {
          beginAvoidance("confirmed obstacle ahead");
          return;
        }
      } else {
        obstacleSamples = 0;
      }
      int speed = !isnan(frontCm) && frontCm < SLOW_DISTANCE_CM
                      ? max(180, (int)cruisePwm - 30)
                      : cruisePwm;
      drive(constrain(speed + leftCorrection, 175, 255),
            constrain(speed + rightCorrection, 175, 255));
    }
    if (now - lastEncoderMs >= 400) {
      lastEncoderMs = now;
      updateEncoderBalance();
    }
    if (now - stateStartMs > 2500) blockedAttempts = 0;
    return;
  }

  if (state == State::REVERSING && now - stateStartMs >= REVERSE_MS) {
    stopMotors();
    if (hasTof && hasServo) {
      scanIndex = 0;
      scanner.write(SCAN_ANGLES[scanIndex]);
      state = State::SCANNING;
      stateStartMs = now;
    } else {
      startTurn(blockedAttempts % 2 != 0, 75.0f);
    }
    return;
  }

  if (state == State::SCANNING && now - stateStartMs >= SERVO_SETTLE_MS) {
    scanDistances[scanIndex] = readTofMedian();
    scanIndex++;
    if (scanIndex >= SCAN_COUNT) {
      selectEscapeDirection();
    } else {
      scanner.write(SCAN_ANGLES[scanIndex]);
      stateStartMs = millis();
    }
    return;
  }

  if (state == State::TURNING) {
    if (now - lastFrontMs >= 110) {
      lastFrontMs = now;
      updateFrontDistance();
      if (!isnan(frontCm) && frontCm <= EMERGENCY_DISTANCE_CM) {
        beginAvoidance("obstacle detected during turn");
        return;
      }
    }
    uint32_t elapsed = now - stateStartMs;
    uint32_t estimatedTurnMs = (uint32_t)(targetTurnDegrees * 7.5f);
    uint32_t hardTimeoutMs = estimatedTurnMs + 450;
    bool finished = hasImu ? accumulatedTurnDegrees >= targetTurnDegrees
                           : elapsed >= estimatedTurnMs;
    if (finished || elapsed >= hardTimeoutMs) {
      stopMotors();
      state = State::CRUISE;
      stateStartMs = now;
      lastFrontMs = 0;
      obstacleSamples = 0;
      Serial.printf("TURN COMPLETE: measured=%.1f degrees, elapsed=%lu ms\n",
                    accumulatedTurnDegrees, (unsigned long)elapsed);
    }
  }
}

void scanI2c() {
  Serial.println("I2C scan:");
  uint8_t found = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  0x%02X%s%s\n", address,
                    address == 0x29 ? "  VL53L0X" : "",
           address == 0x68 ? "  IMU (MPU-6050/MPU-6500)" : "");
      found++;
    }
  }
  if (!found) Serial.println("  no devices found");
}

void printStatus() {
  updateEnvironment(false);
  if (state != State::SCANNING) updateFrontDistance();
  if (hasImu && state != State::TURNING) updateImu();
  Serial.printf("state=%s auto=%s | sonar=%.1fcm tof=%.1fcm front=%.1fcm\n",
                stateName(), autoEnabled ? "on" : "off", sonarCm, centerTofCm, frontCm);
  Serial.printf("gas=%d/4095 alarm=%s threshold=%d | water=%d/4095 stop=%d\n",
                lastGas, gasAlarm ? "YES" : "no", gasThreshold, lastWater, waterThreshold);
  Serial.printf("temperature=%.1fC humidity=%.1f%% | tilt filtered=%.1f raw=%.1fdeg\n",
                temperatureC, humidityPercent, currentTilt, rawTilt);
  Serial.printf("encoders: left=%lu right=%lu | pwm=%u | imu=%s tof=%s servo=%s\n",
                (unsigned long)leftTicks, (unsigned long)rightTicks, cruisePwm,
                hasImu ? "yes" : "no", hasTof ? "yes" : "no", hasServo ? "yes" : "no");
}

void printHelp() {
  Serial.println("\nCommands:");
  Serial.println("  help | status | i2c | dht | imu | gas | water | front | tof | enc");
  Serial.println("  scan | servo 90 | leds | buzzer on | buzzer off | buzzer invert");
  Serial.println("  motor left 150 | motor right 150 | drive 160 160");
  Serial.println("  forward | back | left | right | stop");
  Serial.println("  auto on | auto off | autorun on | autorun off");
  Serial.println("  speed 210 | waterthreshold 2600 | gasthreshold 3400");
  Serial.println("  invertleft | invertright | reversechassis | normalchassis");
  Serial.println("  invertservo | calibrate | resetenc");
}

void runManualDrive(int left, int right) {
  stopEverything();
  drive(left, right);
  manualUntilMs = millis() + MANUAL_RUN_MS;
  Serial.printf("MANUAL: left=%d right=%d; stopping automatically in %lu ms\n",
                left, right, (unsigned long)MANUAL_RUN_MS);
}

void processCommand(String command) {
  command.trim();
  command.toLowerCase();
  if (command.length() == 0) return;

  if (command == "help") printHelp();
  else if (command == "status") printStatus();
  else if (command == "i2c") scanI2c();
  else if (command == "dht") {
    updateEnvironment(true);
    if (isnan(temperatureC) || isnan(humidityPercent)) {
      Serial.println("DHT FAILED: check GPIO23, 3V3, GND, pull-up, and DHT22/DHT11 type.");
    } else Serial.printf("temperature=%.1f C humidity=%.1f %%\n", temperatureC, humidityPercent);
  }
  else if (command == "imu") {
    if (!hasImu) Serial.println("IMU not initialized at I2C address 0x68.");
    else {
      updateImu();
      sensors_event_t acceleration, gyro, temp;
      imu.getEvent(&acceleration, &gyro, &temp);
      Serial.printf("accel x=%.2f y=%.2f z=%.2f m/s2 | gyro z=%.3f rad/s | tilt filtered=%.1f raw=%.1f deg\n",
                    acceleration.acceleration.x, acceleration.acceleration.y,
                    acceleration.acceleration.z, gyro.gyro.z, currentTilt, rawTilt);
    }
  }
  else if (command == "gas") {
    updateEnvironment(false);
    Serial.printf("MQ-4 raw=%d/4095; uncalibrated, NOT methane ppm.\n", lastGas);
  }
  else if (command == "water") {
    updateEnvironment(false);
    Serial.printf("water raw=%d/4095; autonomous stop threshold=%d\n", lastWater, waterThreshold);
  }
  else if (command == "front") {
    updateFrontDistance();
    Serial.printf("HC-SR04=%.1f cm | forward VL53L0X=%.1f cm | used=%.1f cm\n",
                  sonarCm, centerTofCm, frontCm);
  }
  else if (command == "tof") {
    float distance = readTofMedian();
    Serial.printf("VL53L0X=%.1f cm\n", distance);
  }
  else if (command == "enc") {
    Serial.printf("left=%lu right=%lu | levels: left=%d right=%d\n",
                  (unsigned long)leftTicks, (unsigned long)rightTicks,
                  digitalRead(Pin::LEFT_ENCODER), digitalRead(Pin::RIGHT_ENCODER));
  }
  else if (command == "resetenc") {
    detachInterrupt(digitalPinToInterrupt(Pin::LEFT_ENCODER));
    detachInterrupt(digitalPinToInterrupt(Pin::RIGHT_ENCODER));
    leftTicks = rightTicks = 0;
    previousLeftTicks = previousRightTicks = 0;
    encoderHasMoved = false;
    attachInterrupt(digitalPinToInterrupt(Pin::LEFT_ENCODER), onLeftEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(Pin::RIGHT_ENCODER), onRightEncoder, CHANGE);
    Serial.println("Encoder counts reset.");
  }
  else if (command == "scan") {
    stopEverything();
    if (!hasServo || !hasTof) {
      Serial.println("Directional scan requires both the SG90 and VL53L0X.");
    } else {
      for (uint8_t i = 0; i < SCAN_COUNT; i++) {
        scanner.write(SCAN_ANGLES[i]);
        delay(SERVO_SETTLE_MS);
        scanDistances[i] = readTofMedian();
        Serial.printf("%3d degrees -> %.1f cm\n", SCAN_ANGLES[i], scanDistances[i]);
      }
      scanner.write(SERVO_CENTER);
    }
  }
  else if (command.startsWith("servo ")) {
    stopEverything();
    if (!hasServo) Serial.println("Servo initialization failed.");
    else {
      int angle = constrain(command.substring(6).toInt(), 0, 180);
      scanner.write(angle);
      Serial.printf("servo=%d degrees\n", angle);
    }
  }
  else if (command == "leds") {
    stopEverything();
    digitalWrite(Pin::RED, HIGH);
    delay(350);
    digitalWrite(Pin::RED, LOW);
    digitalWrite(Pin::GREEN, HIGH);
    delay(350);
    digitalWrite(Pin::GREEN, LOW);
    Serial.println("Flashed red, then green.");
  }
  else if (command == "buzzer on") { setBuzzer(true); Serial.println("buzzer=on"); }
  else if (command == "buzzer off") { setBuzzer(false); Serial.println("buzzer=off"); }
  else if (command == "buzzer invert") {
    buzzerActiveLow = !buzzerActiveLow;
    setBuzzer(false);
    Serial.printf("buzzer active-low=%s\n", buzzerActiveLow ? "yes" : "no");
  }
  else if (command == "forward") runManualDrive(cruisePwm, cruisePwm);
  else if (command == "back") runManualDrive(-cruisePwm, -cruisePwm);
  else if (command == "left") runManualDrive(-cruisePwm, cruisePwm);
  else if (command == "right") runManualDrive(cruisePwm, -cruisePwm);
  else if (command.startsWith("motor left ")) {
    int speed = constrain(command.substring(11).toInt(), -255, 255);
    runManualDrive(speed, 0);
  }
  else if (command.startsWith("motor right ")) {
    int speed = constrain(command.substring(12).toInt(), -255, 255);
    runManualDrive(0, speed);
  }
  else if (command.startsWith("drive ")) {
    int left = 0, right = 0;
    if (sscanf(command.c_str(), "drive %d %d", &left, &right) == 2) {
      runManualDrive(constrain(left, -255, 255), constrain(right, -255, 255));
    } else Serial.println("usage: drive 160 160");
  }
  else if (command == "stop" || command == "auto off") {
    stopEverything();
    Serial.println("STOPPED");
  }
  else if (command == "auto on") startAutonomy();
  else if (command == "autorun on") {
    preferences.putBool("autorun", true);
    Serial.println("Saved: autonomous mode starts five seconds after every boot.");
  }
  else if (command == "autorun off") {
    preferences.putBool("autorun", false);
    pendingAutorun = false;
    Serial.println("Saved: automatic startup disabled.");
  }
  else if (command.startsWith("speed ")) {
    cruisePwm = constrain(command.substring(6).toInt(), 90, 255);
    Serial.printf("cruise PWM=%u/255\n", cruisePwm);
  }
  else if (command.startsWith("waterthreshold ")) {
    waterThreshold = constrain(command.substring(15).toInt(), 1, 4095);
    Serial.printf("water stop threshold=%d\n", waterThreshold);
  }
  else if (command.startsWith("gasthreshold ")) {
    gasThreshold = constrain(command.substring(13).toInt(), 1, 4095);
    Serial.printf("gas alarm threshold=%d (raw ADC; not ppm)\n", gasThreshold);
  }
  else if (command == "invertleft") {
    stopEverything();
    leftInverted = !leftInverted;
    preferences.putBool("invleft", leftInverted);
    Serial.printf("left motor inverted=%s; saved\n", leftInverted ? "yes" : "no");
  }
  else if (command == "invertright") {
    stopEverything();
    rightInverted = !rightInverted;
    preferences.putBool("invright", rightInverted);
    Serial.printf("right motor inverted=%s; saved\n", rightInverted ? "yes" : "no");
  }
  else if (command == "reversechassis") {
    stopEverything();
    leftInverted = true;
    rightInverted = true;
    preferences.putBool("invleft", true);
    preferences.putBool("invright", true);
    Serial.println("Reversed-chassis orientation saved: left=yes right=yes");
  }
  else if (command == "normalchassis") {
    stopEverything();
    leftInverted = false;
    rightInverted = false;
    preferences.putBool("invleft", false);
    preferences.putBool("invright", false);
    Serial.println("Normal-chassis orientation saved: left=no right=no");
  }
  else if (command == "invertservo") {
    lowAngleIsLeft = !lowAngleIsLeft;
    preferences.putBool("invservo", lowAngleIsLeft);
    Serial.printf("low servo angles point %s; saved\n", lowAngleIsLeft ? "LEFT" : "RIGHT");
  }
  else if (command == "calibrate") {
    stopEverything();
    Serial.println("Hold the rover still and level...");
    calibrateImu();
    Serial.printf("IMU calibrated; pitch=%.1f roll=%.1f gyro bias=%.4f\n",
                  initialPitch, initialRoll, gyroBiasZ);
  }
  else Serial.println("Unknown command. Type: help");
}

void setup() {
  Serial.begin(115200);
  bootMs = millis();

  pinMode(Pin::LEFT_IN1, OUTPUT);
  pinMode(Pin::LEFT_IN2, OUTPUT);
  pinMode(Pin::RIGHT_IN1, OUTPUT);
  pinMode(Pin::RIGHT_IN2, OUTPUT);
  digitalWrite(Pin::LEFT_IN1, LOW);
  digitalWrite(Pin::LEFT_IN2, LOW);
  digitalWrite(Pin::RIGHT_IN1, LOW);
  digitalWrite(Pin::RIGHT_IN2, LOW);

  pinMode(Pin::TRIG, OUTPUT);
  pinMode(Pin::ECHO, INPUT);
  digitalWrite(Pin::TRIG, LOW);
  pinMode(Pin::LEFT_ENCODER, INPUT);   // External pull-ups are mandatory.
  pinMode(Pin::RIGHT_ENCODER, INPUT);  // GPIO34/GPIO35 have no internal pull-ups.
  pinMode(Pin::RED, OUTPUT);
  pinMode(Pin::GREEN, OUTPUT);
  pinMode(Pin::BUZZER, OUTPUT);
  digitalWrite(Pin::RED, LOW);
  digitalWrite(Pin::GREEN, LOW);
  setBuzzer(false);

  analogReadResolution(12);
  analogSetPinAttenuation(Pin::GAS, ADC_11db);
  analogSetPinAttenuation(Pin::WATER, ADC_11db);

  Wire.begin(Pin::SDA, Pin::SCL);
  Wire.setClock(100000);
  dht.begin();
  preferences.begin("rovertest", false);
  // One-time migration for the chassis that is mounted 180 degrees around.
  // Afterwards, reversechassis/normalchassis and per-side commands stay saved.
  if (!preferences.getBool("reversev2", false)) {
    leftInverted = true;
    rightInverted = true;
    preferences.putBool("invleft", true);
    preferences.putBool("invright", true);
    preferences.putBool("reversev2", true);
  } else {
    leftInverted = preferences.getBool("invleft", true);
    rightInverted = preferences.getBool("invright", true);
  }
  lowAngleIsLeft = preferences.getBool("invservo", false);

  scanner.setPeriodHertz(50);
  int servoChannel = scanner.attach(Pin::SERVO, 500, 2400);
  hasServo = servoChannel >= 0 && scanner.attached();
  if (hasServo) scanner.write(SERVO_CENTER);

  bool leftPwmOk = attachMotorPwm(Pin::LEFT_PWM, LEFT_PWM_CHANNEL);
  bool rightPwmOk = attachMotorPwm(Pin::RIGHT_PWM, RIGHT_PWM_CHANNEL);
  stopMotors();

  attachInterrupt(digitalPinToInterrupt(Pin::LEFT_ENCODER), onLeftEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Pin::RIGHT_ENCODER), onRightEncoder, CHANGE);

  // Read WHO_AM_I first so clone boards and MPU-6500-compatible modules are
  // easier to diagnose. 0x68 is MPU-6050; 0x70 is MPU-6500.
  Wire.beginTransmission(0x68);
  Wire.write(0x75);
  uint8_t imuI2cStatus = Wire.endTransmission(false);
  Serial.printf("IMU I2C status: %u\n", imuI2cStatus);
  if (imuI2cStatus == 0) {
    uint8_t received = Wire.requestFrom((uint8_t)0x68, (uint8_t)1);
    if (received == 1) {
      uint8_t whoAmI = Wire.read();
      Serial.printf("IMU WHO_AM_I: 0x%02X (0x68=MPU-6050, 0x70=MPU-6500)\n",
                    whoAmI);
    }
  }

  for (uint8_t attempt = 1; attempt <= 10 && !hasImu; ++attempt) {
    hasImu = imu.begin(0x68, &Wire);
    if (!hasImu) {
      Serial.printf("IMU initialization retry %u/10\n", attempt);
      delay(300);
    }
  }
  if (hasImu) {
    imu.setAccelerometerRange(MPU6050_RANGE_8_G);
    imu.setGyroRange(MPU6050_RANGE_500_DEG);
    imu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    calibrateImu();
  }
  hasTof = tof.begin();

  delay(250);
  Serial.println("\n=== COAL ROVER DIAGNOSTICS + AUTONOMY ===");
  Serial.printf("PWM: left=%s right=%s | servo=%s | IMU=%s | VL53L0X=%s\n",
                leftPwmOk ? "ok" : "FAILED", rightPwmOk ? "ok" : "FAILED",
                hasServo ? "ok" : "FAILED", hasImu ? "ok" : "missing",
                hasTof ? "ok" : "missing");
  Serial.printf("motor inversion: left=%s right=%s | low servo angles=%s\n",
                leftInverted ? "yes" : "no", rightInverted ? "yes" : "no",
                lowAngleIsLeft ? "LEFT" : "RIGHT");
  scanI2c();
  updateEnvironment(true);
  printHelp();

  if (!leftPwmOk || !rightPwmOk) {
    emergencyStop("motor PWM initialization failed");
    return;
  }

  pendingAutorun = preferences.getBool("autorun", false);
  if (pendingAutorun) {
    Serial.println("WARNING: saved autorun enabled; type stop within five seconds.");
    bootMs = millis();
  } else {
    Serial.println("Motors are stopped. Test components, then type: auto on");
  }
}

void loop() {
  while (Serial.available()) {
    char character = (char)Serial.read();
    if (character == '\n' || character == '\r') {
      if (serialLine.length()) {
        processCommand(serialLine);
        serialLine = "";
      }
    } else if (serialLine.length() < 96) {
      serialLine += character;
    }
  }

  uint32_t now = millis();
  if (pendingAutorun && now - bootMs >= AUTORUN_DELAY_MS) startAutonomy();

  if (manualUntilMs && (int32_t)(now - manualUntilMs) >= 0) {
    stopMotors();
    manualUntilMs = 0;
    Serial.println("Manual motor test complete; motors stopped.");
  }

  updateAutonomy();

  if (autoEnabled && now - lastTelemetryMs >= 1200) {
    lastTelemetryMs = now;
    Serial.printf("AUTO %s | front=%.1fcm gas=%d water=%d tilt=%.1f enc=%lu/%lu\n",
                  stateName(), frontCm, lastGas, lastWater, currentTilt,
                  (unsigned long)leftTicks, (unsigned long)rightTicks);
  }
  delay(2);
}
