/*
  DEEPTRACK — FOUR-WHEEL MINE RESCUE ROVER
  Sensor + safety + autonomous obstacle avoidance for ESP32

  Communication:
    ESP-NOW broadcast  → Gateway ESP32 (binary TelemetryPacket, ~200m range)
    Serial JSON        → Wokwi serial monitor / optional socat bridge
    Serial (USB)       → debug output

  Sensors:
    DHT22        → GPIO23              (temperature / humidity)
    MQ-4         → GPIO34 (analog)     (methane / CH4 gas sensor)
    HC-SR04      → TRIG 19 / ECHO 18   (mounted on SG90 scan servo)
    MPU6050      → SDA 21 / SCL 22     (orientation / tilt)
    Left encoder → GPIO32              (wheel pulse count)
    Right encoder→ GPIO35              (wheel pulse count)
    VL53L0X sim  → GPIO39 / VN (analog)(secondary front distance)
    Water sensor → GPIO36 / VP (analog)(flood detection)

  Actuators:
    SG90 scan servo → GPIO13
    Buzzer           → GPIO4
    Red LED          → GPIO26 (DANGER)
    Green LED        → GPIO27 (NORMAL)
    Left  motor PWM  → GPIO25
    Right motor PWM  → GPIO14

  Obstacle avoidance sequence:
    HALT → REVERSE → LOOK_RIGHT → LOOK_LEFT → TURN → resume
*/

#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ESP32Servo.h>
#include "telemetry_packet.h"

// ---------- ESP-NOW ----------
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ---------- Pin map ----------
#define DHT_PIN            23
#define DHT_TYPE           DHT22

#define GAS_PIN            34
#define TRIG_PIN           19
#define ECHO_PIN           18

#define ENC_LEFT_PIN       32
#define ENC_RIGHT_PIN      35

#define TOF_SIM_PIN        39
#define WATER_PIN          36

#define SERVO_PIN          13
#define BUZZER_PIN         4

#define LED_RED_PIN        26
#define LED_GREEN_PIN      27

#define MOTOR_LEFT_PIN     25
#define MOTOR_RIGHT_PIN    14

// ---------- LEDC motor PWM channels ----------
#define MOTOR_LEFT_CH      0
#define MOTOR_RIGHT_CH     1

// ---------- Servo scan angles ----------
const int SERVO_CENTER      = 90;
const int SERVO_LEFT        = 150;
const int SERVO_RIGHT       = 30;

// ---------- Thresholds ----------
const int   GAS_ALARM_RAW        = 1800;
const float OBSTACLE_STOP_CM     = 20.0;
const float OBSTACLE_SLOW_CM     = 45.0;
const int   TOF_SIM_STOP_RAW     = 300;
const float TILT_WARN_DEG        = 25.0;
const float TILT_STOP_DEG        = 40.0;
const int   WATER_ALARM_RAW      = 2500;
const float TEMP_ALARM_C         = 50.0;
const float HUMIDITY_ALARM_PCT   = 85.0;

const unsigned long DHT_INTERVAL_MS     = 2000;
const unsigned long SENSOR_INTERVAL_MS  = 100;
const unsigned long SWEEP_STEP_MS       = 20;
const unsigned long SERVO_SETTLE_MS     = 300;
const unsigned long AVOID_HALT_MS       = 150;
const unsigned long AVOID_REVERSE_MS    = 400;
const unsigned long AVOID_TURN_MS       = 700;
const unsigned long TELEMETRY_MS        = 500;

// ---------- Odometry ----------
const float WHEEL_DIAMETER_CM     = 6.5;
const int   ENCODER_TICKS_PER_REV = 20;
const float WHEEL_BASE_CM         = 15.0;
const float CM_PER_TICK           = (PI * WHEEL_DIAMETER_CM) / ENCODER_TICKS_PER_REV;
const float SIM_MAX_SPEED_CM_S    = 30.0;

// ---------- Globals ----------
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_MPU6050 mpu;
Servo scanServo;

volatile unsigned long leftPulses  = 0;
volatile unsigned long rightPulses = 0;

float lastTempC = NAN, lastHumidity = NAN;
bool  dhtFault  = false;

float frontDistanceCm = 999;
int   tofSimRaw       = 4095;
int   gasRaw          = 0;
int   waterRaw        = 0;
float tiltDeg         = 0;

int servoAngle = SERVO_CENTER;
int servoDir   = 1;
unsigned long lastSweepStep = 0;

// ---------- Rover state ----------
enum RoverState { NORMAL, SLOW, AVOIDING, DANGER };
RoverState state = NORMAL;

enum AvoidPhase {
  AVOID_NONE,
  AVOID_HALT,
  AVOID_REVERSE,
  AVOID_LOOK_RIGHT,
  AVOID_LOOK_LEFT,
  AVOID_TURN
};
AvoidPhase avoidPhase = AVOID_NONE;
unsigned long avoidPhaseStart = 0;
float rightClearanceCm = 999;
float leftClearanceCm  = 999;
int   turnDir = 0;

enum DangerCause { NONE_, GAS, TILT, WATER, TEMP, HUMIDITY, TRAPPED };
DangerCause dangerCause = NONE_;

unsigned long lastDhtRead    = 0;
unsigned long lastSensorRead = 0;
unsigned long lastTelemetry  = 0;

float robotX = 0, robotY = 0;
float headingDeg = 0;
unsigned long prevLeftPulses = 0, prevRightPulses = 0;
int lastLeftCmdSigned = 0, lastRightCmdSigned = 0;

// ---------- Forward declarations ----------
const char* stateName(RoverState s);
const char* dangerCauseName(DangerCause c);

// ---------- ISRs ----------
void IRAM_ATTR onLeftPulse()  { leftPulses++; }
void IRAM_ATTR onRightPulse() { rightPulses++; }

// ---------- Helpers ----------
float readUltrasonicCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (duration == 0) return 999.0;
  return duration * 0.0343 / 2.0;
}

void setMotors(int leftSpeed, int rightSpeed) {
  lastLeftCmdSigned  = leftSpeed;
  lastRightCmdSigned = rightSpeed;
  ledcWrite(MOTOR_LEFT_CH,  constrain(abs(leftSpeed),  0, 255));
  ledcWrite(MOTOR_RIGHT_CH, constrain(abs(rightSpeed), 0, 255));
}

void soundBuzzer(bool on, int freq = 2000) {
  if (on) tone(BUZZER_PIN, freq);
  else    noTone(BUZZER_PIN);
}

void setStatusLeds(bool danger) {
  digitalWrite(LED_RED_PIN,   danger ? HIGH : LOW);
  digitalWrite(LED_GREEN_PIN, danger ? LOW  : HIGH);
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(300);

  // WiFi STA mode for ESP-NOW (no AP connection needed)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // ESP-NOW init
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
  } else {
    // Register broadcast peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("ESP-NOW add peer failed");
    } else {
      Serial.println("ESP-NOW transmitter ready");
    }
  }

  dht.begin();
  Wire.begin();

  if (!mpu.begin()) {
    Serial.println("WARN: MPU6050 not detected");
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(GAS_PIN, INPUT);

  pinMode(ENC_LEFT_PIN, INPUT_PULLUP);
  pinMode(ENC_RIGHT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_LEFT_PIN),  onLeftPulse,  RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_RIGHT_PIN), onRightPulse, RISING);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  setStatusLeds(false);

  ledcSetup(MOTOR_LEFT_CH,  5000, 8);
  ledcAttachPin(MOTOR_LEFT_PIN, MOTOR_LEFT_CH);
  ledcSetup(MOTOR_RIGHT_CH, 5000, 8);
  ledcAttachPin(MOTOR_RIGHT_PIN, MOTOR_RIGHT_CH);

  scanServo.setPeriodHertz(50);
  scanServo.attach(SERVO_PIN, 500, 2400);
  scanServo.write(servoAngle);

  Serial.println("Mine Rescue Rover boot OK");
}

// ---------- Sensor reads ----------
void readEnvironment() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) { dhtFault = true; }
  else { dhtFault = false; lastHumidity = h; lastTempC = t; }
}

void readObstacleSensors() {
  frontDistanceCm = readUltrasonicCm();
  tofSimRaw = analogRead(TOF_SIM_PIN);
}

void readGasSensor()   { gasRaw   = analogRead(GAS_PIN); }
void readWaterSensor() { waterRaw = analogRead(WATER_PIN); }

void readTilt() {
  sensors_event_t a, g, temp;
  if (!mpu.getEvent(&a, &g, &temp)) return;
  float ax = a.acceleration.x, ay = a.acceleration.y, az = a.acceleration.z;
  tiltDeg = atan2(sqrt(ax * ax + ay * ay), az) * 180.0 / PI;
}

bool frontBlocked() {
  return (frontDistanceCm <= OBSTACLE_STOP_CM) || (tofSimRaw <= TOF_SIM_STOP_RAW);
}

// ---------- Odometry ----------
void updateOdometry() {
  noInterrupts();
  unsigned long lp = leftPulses, rp = rightPulses;
  interrupts();

  long deltaLeft  = (long)(lp - prevLeftPulses);
  long deltaRight = (long)(rp - prevRightPulses);
  prevLeftPulses  = lp;
  prevRightPulses = rp;

  float distLeft, distRight;

  if (deltaLeft == 0 && deltaRight == 0) {
    float dt = SENSOR_INTERVAL_MS / 1000.0f;
    distLeft  = (lastLeftCmdSigned  / 255.0f) * SIM_MAX_SPEED_CM_S * dt;
    distRight = (lastRightCmdSigned / 255.0f) * SIM_MAX_SPEED_CM_S * dt;
  } else {
    distLeft  = deltaLeft  * CM_PER_TICK;
    distRight = deltaRight * CM_PER_TICK;
    if (lastLeftCmdSigned  < 0) distLeft  = -distLeft;
    if (lastRightCmdSigned < 0) distRight = -distRight;
    if (lastLeftCmdSigned  == 0) distLeft  = 0;
    if (lastRightCmdSigned == 0) distRight = 0;
  }

  float distCenter      = (distLeft + distRight) / 2.0f;
  float deltaHeadingDeg = (distRight - distLeft) / WHEEL_BASE_CM * (180.0f / PI);
  float headingRad      = headingDeg * PI / 180.0f;

  robotX += distCenter * cos(headingRad);
  robotY += distCenter * sin(headingRad);
  headingDeg += deltaHeadingDeg;
  while (headingDeg >= 360) headingDeg -= 360;
  while (headingDeg < 0)    headingDeg += 360;
}

// ---------- Cruise sweep ----------
void updateCruiseSweep() {
  if (avoidPhase != AVOID_NONE) return;
  if (millis() - lastSweepStep < SWEEP_STEP_MS) return;
  lastSweepStep = millis();

  servoAngle += servoDir * 2;
  if (servoAngle >= SERVO_LEFT)  { servoAngle = SERVO_LEFT;  servoDir = -1; }
  if (servoAngle <= SERVO_RIGHT) { servoAngle = SERVO_RIGHT; servoDir = 1;  }
  scanServo.write(servoAngle);
}

// ---------- Obstacle-avoidance state machine ----------
void runAvoidance() {
  unsigned long elapsed = millis() - avoidPhaseStart;

  switch (avoidPhase) {
    case AVOID_NONE:
      if (frontBlocked()) {
        avoidPhase = AVOID_HALT;
        avoidPhaseStart = millis();
        setMotors(0, 0);
      }
      break;

    case AVOID_HALT:
      setMotors(0, 0);
      if (elapsed >= AVOID_HALT_MS) {
        avoidPhase = AVOID_REVERSE;
        avoidPhaseStart = millis();
      }
      break;

    case AVOID_REVERSE:
      setMotors(-120, -120);
      if (elapsed >= AVOID_REVERSE_MS) {
        setMotors(0, 0);
        scanServo.write(SERVO_RIGHT);
        avoidPhase = AVOID_LOOK_RIGHT;
        avoidPhaseStart = millis();
      }
      break;

    case AVOID_LOOK_RIGHT:
      setMotors(0, 0);
      if (elapsed >= SERVO_SETTLE_MS) {
        rightClearanceCm = readUltrasonicCm();
        scanServo.write(SERVO_LEFT);
        avoidPhase = AVOID_LOOK_LEFT;
        avoidPhaseStart = millis();
      }
      break;

    case AVOID_LOOK_LEFT:
      setMotors(0, 0);
      if (elapsed >= SERVO_SETTLE_MS) {
        leftClearanceCm = readUltrasonicCm();
        bool rightOpen = rightClearanceCm > OBSTACLE_STOP_CM;
        bool leftOpen  = leftClearanceCm  > OBSTACLE_STOP_CM;

        if (!rightOpen && !leftOpen) {
          dangerCause = TRAPPED;
          avoidPhase = AVOID_NONE;
          scanServo.write(SERVO_CENTER);
        } else {
          turnDir = (rightClearanceCm >= leftClearanceCm) ? 1 : -1;
          avoidPhase = AVOID_TURN;
          avoidPhaseStart = millis();
        }
      }
      break;

    case AVOID_TURN:
      if (turnDir > 0) setMotors(160, -160);
      else              setMotors(-160, 160);
      if (elapsed >= AVOID_TURN_MS) {
        setMotors(0, 0);
        scanServo.write(SERVO_CENTER);
        servoAngle = SERVO_CENTER;
        avoidPhase = AVOID_NONE;
      }
      break;
  }
}

// ---------- Decision / arbitration ----------
void decideState() {
  dangerCause = NONE_;

  if (gasRaw >= GAS_ALARM_RAW)                         dangerCause = GAS;
  else if (waterRaw >= WATER_ALARM_RAW)                dangerCause = WATER;
  else if (tiltDeg >= TILT_STOP_DEG)                   dangerCause = TILT;
  else if (!dhtFault && lastTempC >= TEMP_ALARM_C)     dangerCause = TEMP;
  else if (!dhtFault && lastHumidity >= HUMIDITY_ALARM_PCT) dangerCause = HUMIDITY;

  if (dangerCause != NONE_) {
    state = DANGER;
    avoidPhase = AVOID_NONE;
    return;
  }

  runAvoidance();

  if (dangerCause == TRAPPED) { state = DANGER; return; }
  if (avoidPhase != AVOID_NONE) { state = AVOIDING; return; }

  bool slow = (frontDistanceCm <= OBSTACLE_SLOW_CM) || (tiltDeg >= TILT_WARN_DEG);
  state = slow ? SLOW : NORMAL;
}

void actOnState() {
  bool danger = (state == DANGER);
  setStatusLeds(danger);

  int buzzFreq = 2000;
  switch (dangerCause) {
    case GAS:      buzzFreq = 2500; break;
    case TILT:     buzzFreq = 3000; break;
    case WATER:    buzzFreq = 1800; break;
    case TEMP:     buzzFreq = 2700; break;
    case HUMIDITY: buzzFreq = 1600; break;
    case TRAPPED:  buzzFreq = 1200; break;
    default:       buzzFreq = 2000; break;
  }
  soundBuzzer(danger, buzzFreq);

  switch (state) {
    case NORMAL:   setMotors(200, 200); break;
    case SLOW:     setMotors(110, 110); break;
    case AVOIDING: break; // driven by runAvoidance()
    case DANGER:
      switch (dangerCause) {
        case GAS:  case TEMP: setMotors(-150, -150); break;
        default:              setMotors(0, 0);       break;
      }
      break;
  }
}

// ---------- Telemetry ----------
const char* stateName(RoverState s) {
  switch (s) {
    case NORMAL:   return "NORMAL";
    case SLOW:     return "SLOW";
    case AVOIDING: return "AVOIDING";
    case DANGER:   return "DANGER";
  }
  return "UNKNOWN";
}

const char* dangerCauseName(DangerCause c) {
  switch (c) {
    case GAS:      return "GAS";
    case TILT:     return "TILT";
    case WATER:    return "WATER";
    case TEMP:     return "TEMP";
    case HUMIDITY: return "HUMIDITY";
    case TRAPPED:  return "TRAPPED";
    default:       return "NONE";
  }
}

void printTelemetry() {
  noInterrupts();
  unsigned long lp = leftPulses, rp = rightPulses;
  interrupts();

  // Clean single-line JSON for Serial monitor + gateway bridge
  char json[256];
  snprintf(json, sizeof(json),
    "{\"t\":%.1f,\"h\":%.1f,\"gas\":%d,\"dist\":%.1f,\"water\":%d,\"tilt\":%.1f,\"x\":%.1f,\"y\":%.1f,\"hd\":%.0f,\"st\":\"%s\",\"danger\":\"%s\"}",
    dhtFault ? -999.0f : lastTempC,
    dhtFault ? -999.0f : lastHumidity,
    gasRaw, frontDistanceCm,
    waterRaw, tiltDeg,
    robotX, robotY, headingDeg,
    stateName(state),
    dangerCauseName(dangerCause)
  );

  Serial.println(json);

  // ESP-NOW binary packet to gateway
  TelemetryPacket pkt;
  pkt.tempC       = dhtFault ? -999.0f : lastTempC;
  pkt.humidity    = dhtFault ? -999.0f : lastHumidity;
  pkt.gasRaw      = (int16_t)gasRaw;
  pkt.frontCm     = frontDistanceCm;
  pkt.tofRaw      = (int16_t)tofSimRaw;
  pkt.tiltDeg     = tiltDeg;
  pkt.waterRaw    = (int16_t)waterRaw;
  pkt.encL        = (uint32_t)lp;
  pkt.encR        = (uint32_t)rp;
  pkt.x           = robotX;
  pkt.y           = robotY;
  pkt.heading     = headingDeg;
  pkt.state       = (uint8_t)state;
  pkt.dangerCause = (uint8_t)dangerCause;

  esp_now_send(broadcastAddress, (uint8_t*)&pkt, sizeof(pkt));
}

// ---------- Main loop ----------
void loop() {
  unsigned long now = millis();

  if (now - lastDhtRead >= DHT_INTERVAL_MS) {
    lastDhtRead = now;
    readEnvironment();
  }

  if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
    lastSensorRead = now;
    readObstacleSensors();
    readGasSensor();
    readWaterSensor();
    readTilt();

    decideState();
    updateOdometry();
    actOnState();
  }

  updateCruiseSweep();

  if (now - lastTelemetry >= TELEMETRY_MS) {
    lastTelemetry = now;
    printTelemetry();
  }
}