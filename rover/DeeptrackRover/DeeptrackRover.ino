// DEEPTRACK production rover firmware.
// Board: ESP32 Dev Module with Arduino-ESP32 3.3.x.
//
// RoverDiagnostics and TB6612Test remain separate known-good bench sketches.
// This mission sketch starts disarmed, treats unknown/stale clearance as
// blocked, never reverses autonomously, and refuses auto mode until the
// geometry-dependent values have been measured and explicitly stored.

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_VL53L0X.h>
#include <Arduino.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_arduino_version.h>
#include <esp_now.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <math.h>

#include "../../firmware/shared/DeeptrackFrontSafety.h"
#include "../../firmware/shared/DeeptrackHardware.h"
#include "../../firmware/shared/DeeptrackProtocol.h"
#include "../../firmware/shared/DeeptrackRadioConfig.h"
#include "../../firmware/shared/DeeptrackRuntimeSafety.h"
#include "../../firmware/shared/DeeptrackThresholds.h"

namespace Pin = DeepTrack::Hardware::Rover;
namespace Protocol = DeepTrack::Protocol;
namespace Radio = DeepTrack::RadioConfig;
namespace Safety = DeepTrack::Safety;
namespace Threshold = DeepTrack::Thresholds;

bool leftInverted = true;
bool rightInverted = true;
constexpr uint32_t MOTOR_PWM_HZ = 18000;
constexpr uint8_t MOTOR_PWM_BITS = 8;
constexpr uint8_t LEFT_PWM_CHANNEL_V2 = 4;
constexpr uint8_t RIGHT_PWM_CHANNEL_V2 = 5;
constexpr size_t MAX_SERIAL_LINE = 128;

struct UltrasonicSample { bool valid; float cm; uint32_t pulse_us; };
struct TofSample { bool valid; uint16_t mm; uint8_t status; };
struct Calibration {
  uint16_t chassis_width_mm = 0;
  uint16_t turn_90_ms = 0;
  uint16_t water_dry = 0;
  uint16_t water_wet = 0;
  uint16_t gas_baseline = 0;
  uint16_t track_width_mm = 0;
  uint16_t micrometers_per_tick = 0;
  int16_t pitch_offset_cdeg = 0;
  int16_t roll_offset_cdeg = 0;
  float gyro_z_bias_dps = 0.0f;
  uint8_t servo_center_deg = 0;
  bool servo_low_is_left = false;
  bool servo_direction_known = false;
  bool motor_direction_known = false;
  bool imu_level_known = false;
  bool gyro_bias_known = false;
};
struct ScanSample { uint8_t angle; uint16_t mm; uint8_t status; bool valid; };

enum class AutoPhase : uint8_t {
  IDLE,
  ADVANCE,
  MOVE_SERVO,
  WAIT_SERVO,
  MEASURE,
  TURN,
  CENTER_WAIT,
  VERIFY,
  STUCK,
};

Safety::FrontSafetyGate frontGate(Threshold::FRONT_STOP_CM,
                                  Threshold::FRONT_CLEAR_CM,
                                  Threshold::FRONT_STALE_AFTER_MS);
Adafruit_MPU6050 mpu;
Adafruit_VL53L0X tof;
DHT dht(Pin::DHT_DATA, DHT22);
Servo scanServo;
Preferences preferences;
portMUX_TYPE radioMux = portMUX_INITIALIZER_UNLOCKED;

volatile int32_t leftTicks = 0;
volatile int32_t rightTicks = 0;
volatile int8_t leftEncoderDirection = 0;
volatile int8_t rightEncoderDirection = 0;
volatile bool commandPending = false;
Protocol::CommandPacket pendingCommand{};
uint32_t pendingCommandReceivedAtMs = 0;
volatile bool diagnosticPending = false;
Protocol::DiagnosticCommandPacket pendingDiagnostic{};
volatile int8_t lastGatewayRssiDbm = Protocol::RSSI_UNAVAILABLE_DBM;

bool motorPwmReady = false;
bool imuReady = false;
bool tofReady = false;
bool servoReady = false;
bool radioReady = false;
bool brakeLatched = true;
bool imuSampleValid = false;
bool armed = false;
bool gasAdvisory = false;
bool waterContact = false;
bool tiltStopped = false;
bool stallSuspected = false;
bool buzzerOn = false;
bool gyroCalibrationActive = false;
bool observationOnlyScan = false;

uint32_t activeSession = 0;
uint32_t lastCommandSequence = 0;
uint32_t outgoingSequence = 0;
uint32_t lastValidCommandAtMs = 0;
uint16_t acceptedCommandTtlMs = 0;
uint32_t lastFrontAtMs = 0;
uint32_t lastGasAtMs = 0;
uint32_t lastWaterAtMs = 0;
uint32_t lastImuAtMs = 0;
uint32_t lastTofValidAtMs = 0;
uint32_t lastCenterTofAtMs = 0;
uint32_t lastDhtAtMs = 0;
uint32_t lastDhtValidAtMs = 0;
uint32_t lastTelemetryAtMs = 0;
uint32_t lastRampAtMs = 0;
uint32_t lastLeftEncoderMotionAtMs = 0;
uint32_t lastRightEncoderMotionAtMs = 0;
uint32_t benchMotionUntilMs = 0;
uint32_t tiltExceededAtMs = 0;
uint32_t scanId = 0;
uint32_t lastScanCompletedAtMs = 0;
uint32_t lastBuzzerToggleAtMs = 0;
uint32_t gyroCalibrationStartedAtMs = 0;
uint32_t diagnosticLedsOffAtMs = 0;
uint32_t diagnosticBuzzerOffAtMs = 0;

int16_t targetLeft = 0;
int16_t targetRight = 0;
int16_t appliedLeft = 0;
int16_t appliedRight = 0;
int32_t lastStallLeftTicks = 0;
int32_t lastStallRightTicks = 0;
float pitchDeg = NAN;
float rollDeg = NAN;
float rawPitchDeg = NAN;
float rawRollDeg = NAN;
float headingDeg = 0.0f;
float temperatureC = NAN;
float humidityPct = NAN;
uint16_t gasRaw = 0;
uint16_t gasPinMv = 0;
uint16_t waterRaw = 0;
uint16_t waterPinMv = 0;
uint16_t latestCenterTofMm = Protocol::UNKNOWN_DISTANCE_MM;
uint8_t latestCenterTofStatus = UINT8_MAX;
float gasEma = NAN;
double gyroCalibrationSum = 0.0;
double gyroCalibrationSumSquares = 0.0;
uint16_t gyroCalibrationSamples = 0;
uint16_t gasWindow[Threshold::GAS_MEDIAN_WINDOW]{};
uint8_t gasWindowCount = 0;
uint8_t gasWindowIndex = 0;
uint8_t gasAdvisoryCount = 0;
uint8_t waterContactCount = 0;
uint8_t waterClearCount = 0;
uint8_t currentServoDeg = Threshold::SERVO_CENTER_DEG;
Protocol::DriveState driveState = Protocol::DriveState::STOPPED;
AutoPhase autoPhase = AutoPhase::IDLE;
ScanSample scanSamples[Threshold::SCAN_ANGLE_COUNT]{};
uint8_t scanIndex = 0;
uint32_t autoPhaseAtMs = 0;
int8_t selectedTurnSign = 0;
uint32_t selectedTurnDurationMs = 0;
Calibration calibration;
String serialLine;

void IRAM_ATTR onLeftEncoder() { leftTicks += leftEncoderDirection; }
void IRAM_ATTR onRightEncoder() { rightTicks += rightEncoderDirection; }

bool elapsed(uint32_t now, uint32_t since, uint32_t interval) {
  return now - since >= interval;
}

bool nonzeroBytes(const uint8_t* data, size_t length) {
  uint8_t combined = 0;
  for (size_t i = 0; i < length; ++i) combined |= data[i];
  return combined != 0;
}

bool macEquals(const uint8_t* first, const uint8_t* second) {
  return first && second && memcmp(first, second, 6) == 0;
}

bool radioConfigurationValid() {
  return Radio::ENABLED && Radio::CHANNEL >= 1 && Radio::CHANNEL <= 14 &&
         nonzeroBytes(Radio::ROVER_MAC, 6) &&
         nonzeroBytes(Radio::GATEWAY_MAC, 6) && nonzeroBytes(Radio::PMK, 16) &&
         nonzeroBytes(Radio::LMK, 16);
}

void fillHeader(Protocol::PacketHeader& header, Protocol::MessageType type) {
  header.magic = Protocol::PACKET_MAGIC;
  header.version = Protocol::PROTOCOL_VERSION;
  header.type = static_cast<uint8_t>(type);
  header.session_id = activeSession;
  header.sequence = ++outgoingSequence;
  header.sender_uptime_ms = millis();
}

template <typename Packet>
bool sendPacket(const Packet& packet) {
  if (!radioReady) return false;
  return esp_now_send(Radio::GATEWAY_MAC,
                      reinterpret_cast<const uint8_t*>(&packet),
                      sizeof(packet)) == ESP_OK;
}

void sendEvent(Protocol::EventCode code, uint8_t severity, int32_t value = 0) {
  Protocol::EventPacket packet{};
  fillHeader(packet.header, Protocol::MessageType::EVENT);
  packet.code = static_cast<uint16_t>(code);
  packet.severity = severity;
  packet.value = value;
  sendPacket(packet);
}

void sendAck(uint32_t commandSequence, Protocol::AckStatus status,
             Protocol::AckReason reason) {
  Protocol::AppAckPacket packet{};
  fillHeader(packet.header, Protocol::MessageType::APP_ACK);
  packet.acknowledged_sequence = commandSequence;
  packet.status = static_cast<uint8_t>(status);
  packet.reason = static_cast<uint8_t>(reason);
  sendPacket(packet);
}

void onRadioSend(const esp_now_send_info_t*, esp_now_send_status_t) {}

void onRadioReceive(const esp_now_recv_info_t* info, const uint8_t* data,
                    int length) {
  if (!info || !data || !macEquals(info->src_addr, Radio::GATEWAY_MAC) ||
      length < static_cast<int>(sizeof(Protocol::PacketHeader))) return;
  Protocol::PacketHeader header{};
  memcpy(&header, data, sizeof(header));
  if (header.magic != Protocol::PACKET_MAGIC ||
      header.version != Protocol::PROTOCOL_VERSION) return;
  if (info->rx_ctrl)
    lastGatewayRssiDbm = static_cast<int8_t>(info->rx_ctrl->rssi);
  if (header.type == static_cast<uint8_t>(Protocol::MessageType::COMMAND) &&
      length == static_cast<int>(sizeof(Protocol::CommandPacket))) {
    portENTER_CRITICAL(&radioMux);
    memcpy(&pendingCommand, data, sizeof(pendingCommand));
    pendingCommandReceivedAtMs = millis();
    commandPending = true;
    portEXIT_CRITICAL(&radioMux);
  } else if (header.type == static_cast<uint8_t>(
                 Protocol::MessageType::DIAGNOSTIC_COMMAND) &&
             length == static_cast<int>(
                 sizeof(Protocol::DiagnosticCommandPacket))) {
    portENTER_CRITICAL(&radioMux);
    memcpy(&pendingDiagnostic, data, sizeof(pendingDiagnostic));
    diagnosticPending = true;
    portEXIT_CRITICAL(&radioMux);
  }
}

bool initializeRadio() {
  if (!radioConfigurationValid()) return false;
  WiFi.mode(WIFI_STA);
  uint8_t actualMac[6]{};
  if (esp_wifi_get_mac(WIFI_IF_STA, actualMac) != ESP_OK ||
      !macEquals(actualMac, Radio::ROVER_MAC)) return false;
  if (esp_wifi_set_channel(Radio::CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK ||
      esp_now_init() != ESP_OK || esp_now_set_pmk(Radio::PMK) != ESP_OK ||
      esp_now_register_recv_cb(onRadioReceive) != ESP_OK ||
      esp_now_register_send_cb(onRadioSend) != ESP_OK) return false;
  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, Radio::GATEWAY_MAC, 6);
  peer.channel = Radio::CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = true;
  memcpy(peer.lmk, Radio::LMK, 16);
  return esp_now_add_peer(&peer) == ESP_OK;
}

void writeMotorPwm(uint8_t pin, uint8_t v2Channel, uint8_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)v2Channel;
  ledcWrite(pin, duty);
#else
  (void)pin;
  ledcWrite(v2Channel, duty);
#endif
}

bool attachMotorPwm(uint8_t pin, uint8_t v2Channel) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)v2Channel;
  return ledcAttach(pin, MOTOR_PWM_HZ, MOTOR_PWM_BITS);
#else
  ledcSetup(v2Channel, MOTOR_PWM_HZ, MOTOR_PWM_BITS);
  ledcAttachPin(pin, v2Channel);
  return true;
#endif
}

bool attachScanServo() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  // The locally patched ESP32Servo MCPWM backend scales the requested pulse
  // down by roughly 100x on this core. Use the core LEDC API directly so an
  // SG90 receives a real 20 ms frame with a 500-2400 us high pulse.
  return ledcAttach(Pin::SERVO, 50, 16);
#else
  scanServo.setPeriodHertz(50);
  return scanServo.attach(Pin::SERVO, 500, 2400) >= 0 &&
         scanServo.attached();
#endif
}

void writeScanServo(uint8_t angle) {
  currentServoDeg = constrain(angle, 0, 180);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  const uint32_t pulseUs = map(currentServoDeg, 0, 180, 500, 2400);
  const uint32_t duty =
      (pulseUs * 65535UL + 10000UL) / 20000UL;
  ledcWrite(Pin::SERVO, duty);
#else
  scanServo.write(currentServoDeg);
#endif
}

void setSideDrive(uint8_t pwm, uint8_t in1, uint8_t in2, uint8_t channel,
                  int16_t request, bool inverted) {
  int16_t physical = constrain(request, -255, 255);
  if (inverted) physical = -physical;
  if (!physical) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    writeMotorPwm(pwm, channel, 0);
    return;
  }
  digitalWrite(in1, physical > 0 ? HIGH : LOW);
  digitalWrite(in2, physical > 0 ? LOW : HIGH);
  writeMotorPwm(pwm, channel, static_cast<uint8_t>(abs(physical)));
}

void activeBrake(bool report = false) {
  const bool wasBrakeLatched = brakeLatched;
  targetLeft = targetRight = 0;
  appliedLeft = appliedRight = 0;
  leftEncoderDirection = rightEncoderDirection = 0;
  benchMotionUntilMs = 0;
  digitalWrite(Pin::LEFT_IN1, HIGH);
  digitalWrite(Pin::LEFT_IN2, HIGH);
  digitalWrite(Pin::RIGHT_IN1, HIGH);
  digitalWrite(Pin::RIGHT_IN2, HIGH);
  if (motorPwmReady) {
    writeMotorPwm(Pin::LEFT_PWM, LEFT_PWM_CHANNEL_V2, 255);
    writeMotorPwm(Pin::RIGHT_PWM, RIGHT_PWM_CHANNEL_V2, 255);
  } else {
    digitalWrite(Pin::LEFT_PWM, HIGH);
    digitalWrite(Pin::RIGHT_PWM, HIGH);
  }
  brakeLatched = true;
  if (driveState != Protocol::DriveState::STUCK)
    driveState = report ? Protocol::DriveState::SAFE_STOP
                        : Protocol::DriveState::STOPPED;
  if (report && !wasBrakeLatched)
    sendEvent(Protocol::EventCode::ACTIVE_BRAKE, 1);
}

int16_t approach(int16_t current, int16_t target, int16_t step) {
  if (current < target) return min<int16_t>(current + step, target);
  if (current > target) return max<int16_t>(current - step, target);
  return current;
}

void serviceMotorRamp() {
  if (brakeLatched) return;
  const uint32_t now = millis();
  if (!elapsed(now, lastRampAtMs, Threshold::MOTOR_RAMP_INTERVAL_MS)) return;
  lastRampAtMs = now;
  appliedLeft = approach(appliedLeft, targetLeft, Threshold::MOTOR_RAMP_STEP);
  appliedRight = approach(appliedRight, targetRight, Threshold::MOTOR_RAMP_STEP);
  leftEncoderDirection = Safety::encoderDirectionForDuty(appliedLeft);
  rightEncoderDirection = Safety::encoderDirectionForDuty(appliedRight);
  setSideDrive(Pin::LEFT_PWM, Pin::LEFT_IN1, Pin::LEFT_IN2,
               LEFT_PWM_CHANNEL_V2, appliedLeft, leftInverted);
  setSideDrive(Pin::RIGHT_PWM, Pin::RIGHT_IN1, Pin::RIGHT_IN2,
               RIGHT_PWM_CHANNEL_V2, appliedRight, rightInverted);
}

UltrasonicSample readFrontUltrasonic() {
  digitalWrite(Pin::TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(Pin::TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(Pin::TRIG, LOW);
  const uint32_t pulse =
      pulseIn(Pin::ECHO, HIGH, Threshold::ULTRASONIC_TIMEOUT_US);
  if (!pulse) return {false, NAN, 0};
  const float cm = pulse * 0.0343f / 2.0f;
  return {cm >= Threshold::FRONT_MIN_CM && cm <= Threshold::FRONT_MAX_CM,
          cm, pulse};
}

TofSample readCurrentTof() {
  if (!tofReady) return {false, Protocol::UNKNOWN_DISTANCE_MM, UINT8_MAX};
  VL53L0X_RangingMeasurementData_t measurement{};
  tof.rangingTest(&measurement, false);
  // Preserve the known-good diagnostic sketch's bounded degraded reading:
  // status 4 is unusable, while other nonzero statuses remain measurable but
  // must be reported as CHECK rather than PASS.
  const bool usable = measurement.RangeStatus != 4 &&
                      measurement.RangeMilliMeter >= Threshold::TOF_MIN_MM &&
                      measurement.RangeMilliMeter <= Threshold::TOF_MAX_MM;
  if (usable && measurement.RangeStatus == 0) lastTofValidAtMs = millis();
  return {usable, measurement.RangeMilliMeter, measurement.RangeStatus};
}

TofSample readTofMedian() {
  TofSample samples[3]{};
  uint8_t count = 0;
  uint8_t lastStatus = UINT8_MAX;
  for (uint8_t i = 0; i < 3; ++i) {
    const TofSample sample = readCurrentTof();
    lastStatus = sample.status;
    if (sample.valid) samples[count++] = sample;
  }
  if (!count)
    return {false, Protocol::UNKNOWN_DISTANCE_MM, lastStatus};
  for (uint8_t i = 0; i < count; ++i) {
    for (uint8_t j = i + 1; j < count; ++j) {
      if (samples[j].mm < samples[i].mm) {
        const TofSample temporary = samples[i];
        samples[i] = samples[j];
        samples[j] = temporary;
      }
    }
  }
  return samples[count / 2];
}

TofSample readCenteredTof() {
  if (!servoReady || !tofReady)
    return {false, Protocol::UNKNOWN_DISTANCE_MM, UINT8_MAX};
  const uint8_t center = calibration.servo_center_deg
                             ? calibration.servo_center_deg
                             : Threshold::SERVO_CENTER_DEG;
  if (currentServoDeg != center) {
    writeScanServo(center);
    delay(Threshold::SERVO_SETTLE_MS);
  }
  const TofSample sample = readTofMedian();
  latestCenterTofMm = sample.valid ? sample.mm : Protocol::UNKNOWN_DISTANCE_MM;
  latestCenterTofStatus = sample.status;
  lastCenterTofAtMs = millis();
  return sample;
}

void beginAutoScan(bool observationOnly = false);

void updateIndicators() {
  const bool frontBlocked = frontGate.snapshot(millis()).forward_blocked;
  const bool stopHazard = waterContact || tiltStopped || stallSuspected ||
                          driveState == Protocol::DriveState::STUCK;
  digitalWrite(Pin::RED, frontBlocked || stopHazard ? HIGH : LOW);
  digitalWrite(Pin::GREEN, !frontBlocked && !stopHazard ? HIGH : LOW);
}

void sampleFront(bool force = false) {
  const uint32_t now = millis();
  if (!force &&
      !elapsed(now, lastFrontAtMs, Threshold::FRONT_SAMPLE_INTERVAL_MS)) {
    frontGate.refresh(now);
    return;
  }
  lastFrontAtMs = now;
  const bool wasBlocked = frontGate.snapshot(now).forward_blocked;
  const UltrasonicSample sample = readFrontUltrasonic();
  frontGate.observe(sample.valid, sample.cm, millis());
  const bool blocked = frontGate.snapshot(millis()).forward_blocked;
  const bool forwardWasRequested =
      Safety::FrontSafetyGate::requestsForward(targetLeft, targetRight);
  const bool manualForwardWasRequested =
      armed && autoPhase == AutoPhase::IDLE &&
      driveState == Protocol::DriveState::MANUAL && forwardWasRequested;
  const bool autoForwardWasRequested =
      armed && autoPhase != AutoPhase::IDLE && forwardWasRequested;
  if (blocked && forwardWasRequested) {
    activeBrake(true);
    if (autoForwardWasRequested) {
      beginAutoScan(false);
    } else if (manualForwardWasRequested && tofReady && servoReady &&
               (!lastScanCompletedAtMs ||
                elapsed(now, lastScanCompletedAtMs,
                        Threshold::MANUAL_SCAN_COOLDOWN_MS))) {
      // Manual mode keeps ownership of motion. The rover only performs a
      // stationary perception sweep and publishes the rays for the map.
      beginAutoScan(true);
    }
  }
  if (!wasBlocked && blocked)
    sendEvent(sample.valid ? Protocol::EventCode::FRONT_BLOCKED
                           : Protocol::EventCode::FRONT_STALE,
              1, sample.valid ? static_cast<int32_t>(sample.cm * 10.0f) : -1);
  updateIndicators();
}

bool requestMotion(int16_t left, int16_t right,
                   Protocol::DriveState requestedState) {
  if (!motorPwmReady) return false;
  if (waterContact || tiltStopped || stallSuspected ||
      gyroCalibrationActive ||
      driveState == Protocol::DriveState::STUCK) {
    activeBrake(true);
    return false;
  }
  const bool manualForwardRequest =
      armed && requestedState == Protocol::DriveState::MANUAL &&
      autoPhase == AutoPhase::IDLE &&
      Safety::FrontSafetyGate::requestsForward(left, right);
  if (Safety::FrontSafetyGate::requestsForward(left, right)) sampleFront(true);
  if (!frontGate.allows(left, right, millis())) {
    activeBrake(true);
    const uint32_t now = millis();
    if (manualForwardRequest && tofReady && servoReady &&
        (!lastScanCompletedAtMs ||
         elapsed(now, lastScanCompletedAtMs,
                 Threshold::MANUAL_SCAN_COOLDOWN_MS))) {
      beginAutoScan(true);
    }
    return false;
  }
  targetLeft = constrain(left, -255, 255);
  targetRight = constrain(right, -255, 255);
  brakeLatched = false;
  driveState = requestedState;
  return true;
}

uint16_t medianGas() {
  uint16_t values[Threshold::GAS_MEDIAN_WINDOW]{};
  const uint8_t count = gasWindowCount;
  for (uint8_t i = 0; i < count; ++i) values[i] = gasWindow[i];
  for (uint8_t i = 1; i < count; ++i) {
    uint16_t value = values[i];
    int8_t j = i - 1;
    while (j >= 0 && values[j] > value) {
      values[j + 1] = values[j];
      --j;
    }
    values[j + 1] = value;
  }
  return count ? values[count / 2] : 0;
}

bool waterCalibrated() {
  return calibration.water_dry && calibration.water_wet &&
         abs(static_cast<int>(calibration.water_wet) -
             static_cast<int>(calibration.water_dry)) >=
             Threshold::WATER_MIN_CALIBRATION_SPAN;
}

bool autoCalibrated() {
  return calibration.chassis_width_mm >= 120 &&
         calibration.chassis_width_mm <= 600 &&
         calibration.turn_90_ms >= 200 && calibration.turn_90_ms <= 1000 &&
         calibration.motor_direction_known &&
         calibration.servo_direction_known &&
         calibration.servo_center_deg >= 80 &&
         calibration.servo_center_deg <= 100;
}

void serviceAnalogSensors() {
  const uint32_t now = millis();
  if (elapsed(now, lastGasAtMs, Threshold::GAS_INTERVAL_MS)) {
    lastGasAtMs = now;
    gasRaw = analogRead(Pin::GAS);
    gasPinMv = analogReadMilliVolts(Pin::GAS);
    gasWindow[gasWindowIndex] = gasRaw;
    gasWindowIndex = (gasWindowIndex + 1) % Threshold::GAS_MEDIAN_WINDOW;
    if (gasWindowCount < Threshold::GAS_MEDIAN_WINDOW) ++gasWindowCount;
    const uint16_t median = medianGas();
    gasEma = isnan(gasEma)
                 ? median
                 : Threshold::GAS_EMA_ALPHA * median +
                       (1.0f - Threshold::GAS_EMA_ALPHA) * gasEma;
    if (calibration.gas_baseline &&
        elapsed(now, 0, Threshold::GAS_BOOT_STABILIZE_MS)) {
      const uint16_t delta =
          max<uint16_t>(Threshold::GAS_MIN_ADVISORY_DELTA,
                        calibration.gas_baseline / 5);
      if (gasEma >= calibration.gas_baseline + delta) {
        if (gasAdvisoryCount < 255) ++gasAdvisoryCount;
      } else if (gasEma <= calibration.gas_baseline + delta / 2) {
        gasAdvisoryCount = 0;
        gasAdvisory = false;
      }
      if (!gasAdvisory &&
          gasAdvisoryCount >= Threshold::GAS_ADVISORY_PERSIST_SAMPLES) {
        gasAdvisory = true;
        sendEvent(Protocol::EventCode::GAS_ADVISORY, 1,
                  static_cast<int32_t>(gasEma));
      }
    }
  }

  if (elapsed(now, lastWaterAtMs, Threshold::WATER_INTERVAL_MS)) {
    lastWaterAtMs = now;
    waterRaw = analogRead(Pin::WATER);
    waterPinMv = analogReadMilliVolts(Pin::WATER);
    if (waterCalibrated()) {
      const bool wetDirectionHigh =
          calibration.water_wet > calibration.water_dry;
      const int span = abs(static_cast<int>(calibration.water_wet) -
                           static_cast<int>(calibration.water_dry));
      const int contactThreshold = wetDirectionHigh
          ? calibration.water_dry +
                span * Threshold::WATER_CONTACT_PERCENT_OF_SPAN / 100
          : calibration.water_dry -
                span * Threshold::WATER_CONTACT_PERCENT_OF_SPAN / 100;
      const int clearThreshold = wetDirectionHigh
          ? calibration.water_dry +
                span * Threshold::WATER_CLEAR_PERCENT_OF_SPAN / 100
          : calibration.water_dry -
                span * Threshold::WATER_CLEAR_PERCENT_OF_SPAN / 100;
      const bool contactNow =
          wetDirectionHigh ? waterRaw >= contactThreshold
                           : waterRaw <= contactThreshold;
      const bool clearNow =
          wetDirectionHigh ? waterRaw <= clearThreshold
                           : waterRaw >= clearThreshold;
      if (contactNow) {
        if (waterContactCount < 255) ++waterContactCount;
        waterClearCount = 0;
      } else if (clearNow) {
        waterContactCount = 0;
        if (waterClearCount < 255) ++waterClearCount;
        if (waterClearCount >= Threshold::WATER_CLEAR_PERSIST_SAMPLES)
          waterContact = false;
      }
      if (!waterContact &&
          waterContactCount >= Threshold::WATER_CONTACT_PERSIST_SAMPLES) {
        waterContact = true;
        armed = false;
        autoPhase = AutoPhase::IDLE;
        observationOnlyScan = false;
        activeBrake(true);
        sendEvent(Protocol::EventCode::WATER_CONTACT, 2, waterRaw);
      }
    }
  }
}

void serviceImu() {
  const uint32_t now = millis();
  if (!imuReady ||
      !elapsed(now, lastImuAtMs, Threshold::IMU_INTERVAL_MS)) return;
  const float dt = lastImuAtMs ? (now - lastImuAtMs) / 1000.0f : 0.0f;
  lastImuAtMs = now;
  sensors_event_t accel{}, gyro{}, temp{};
  if (!mpu.getEvent(&accel, &gyro, &temp)) return;
  imuSampleValid = true;
  rawPitchDeg =
      atan2f(-accel.acceleration.x,
             sqrtf(accel.acceleration.y * accel.acceleration.y +
                   accel.acceleration.z * accel.acceleration.z)) *
      180.0f / PI;
  rawRollDeg =
      atan2f(accel.acceleration.y, accel.acceleration.z) * 180.0f / PI;
  pitchDeg = rawPitchDeg - calibration.pitch_offset_cdeg / 100.0f;
  rollDeg = rawRollDeg - calibration.roll_offset_cdeg / 100.0f;
  const float gyroZDps = gyro.gyro.z * 180.0f / PI;

  if (gyroCalibrationActive) {
    gyroCalibrationSum += gyroZDps;
    gyroCalibrationSumSquares += gyroZDps * gyroZDps;
    if (gyroCalibrationSamples < UINT16_MAX) ++gyroCalibrationSamples;
    if (elapsed(now, gyroCalibrationStartedAtMs,
                Threshold::GYRO_BIAS_CALIBRATION_MS)) {
      const double mean = gyroCalibrationSamples
          ? gyroCalibrationSum / gyroCalibrationSamples : 0.0;
      const double variance = gyroCalibrationSamples
          ? max(0.0, gyroCalibrationSumSquares / gyroCalibrationSamples -
                         mean * mean)
          : 0.0;
      const double standardDeviation = sqrt(variance);
      gyroCalibrationActive = false;
      if (gyroCalibrationSamples >= Threshold::GYRO_BIAS_MIN_SAMPLES &&
          fabs(mean) <= Threshold::GYRO_BIAS_MAX_ABS_DPS &&
          standardDeviation <= Threshold::GYRO_BIAS_MAX_STDDEV_DPS) {
        calibration.gyro_z_bias_dps = static_cast<float>(mean);
        calibration.gyro_bias_known = true;
        preferences.putFloat("gyro_bias", calibration.gyro_z_bias_dps);
        preferences.putBool("gyro_known", true);
        headingDeg = 0.0f;
        Serial.printf(
            "RECORDED gyro Z bias=%.4f dps samples=%u stddev=%.4f dps\n",
            calibration.gyro_z_bias_dps, gyroCalibrationSamples,
            standardDeviation);
      } else {
        Serial.printf(
            "REJECTED gyro calibration: keep rover stationary; samples=%u mean=%.4f stddev=%.4f dps\n",
            gyroCalibrationSamples, mean, standardDeviation);
      }
    }
  } else {
    headingDeg += (gyroZDps - calibration.gyro_z_bias_dps) * dt;
  }
  while (headingDeg < 0) headingDeg += 360.0f;
  while (headingDeg >= 360.0f) headingDeg -= 360.0f;

  const float tilt = max(fabsf(pitchDeg), fabsf(rollDeg));
  if (tilt >= Threshold::TILT_STOP_DEG) {
    if (!tiltExceededAtMs) tiltExceededAtMs = now;
    if (!tiltStopped &&
        elapsed(now, tiltExceededAtMs, Threshold::TILT_STOP_PERSIST_MS)) {
      tiltStopped = true;
      armed = false;
      autoPhase = AutoPhase::IDLE;
      observationOnlyScan = false;
      activeBrake(true);
      sendEvent(Protocol::EventCode::TILT_STOP, 2,
                static_cast<int32_t>(tilt * 100.0f));
    }
  } else if (tilt <= Threshold::TILT_CLEAR_DEG) {
    tiltExceededAtMs = 0;
    tiltStopped = false;
  }
}

void serviceDht() {
  const uint32_t now = millis();
  if (!elapsed(now, lastDhtAtMs, Threshold::DHT_INTERVAL_MS)) return;
  lastDhtAtMs = now;
  const float humidity = dht.readHumidity();
  const float temperature = dht.readTemperature();
  if (!isnan(humidity) && !isnan(temperature)) {
    humidityPct = humidity;
    temperatureC = temperature;
    lastDhtValidAtMs = now;
  }
}

void serviceStallDetection() {
  const uint32_t now = millis();
  // Encoder-based stall inference is unsafe until both motor direction and
  // distance-per-tick have been measured. Before that point encoder noise or
  // a disconnected channel must remain an odometry warning, never a stop.
  if (!calibration.motor_direction_known ||
      !calibration.micrometers_per_tick) {
    lastStallLeftTicks = leftTicks;
    lastStallRightTicks = rightTicks;
    lastLeftEncoderMotionAtMs = now;
    lastRightEncoderMotionAtMs = now;
    stallSuspected = false;
    return;
  }
  const int32_t currentLeftTicks = leftTicks;
  const int32_t currentRightTicks = rightTicks;
  if (!Safety::dutyRequiresEncoderMotion(appliedLeft,
                                         Threshold::STALL_MIN_DUTY)) {
    lastStallLeftTicks = leftTicks;
    lastLeftEncoderMotionAtMs = now;
  } else if (currentLeftTicks != lastStallLeftTicks) {
    lastStallLeftTicks = currentLeftTicks;
    lastLeftEncoderMotionAtMs = now;
  }

  if (!Safety::dutyRequiresEncoderMotion(appliedRight,
                                         Threshold::STALL_MIN_DUTY)) {
    lastStallRightTicks = currentRightTicks;
    lastRightEncoderMotionAtMs = now;
  } else if (currentRightTicks != lastStallRightTicks) {
    lastStallRightTicks = currentRightTicks;
    lastRightEncoderMotionAtMs = now;
  }

  const bool leftStalled = Safety::sideHasStalled(
      appliedLeft, Threshold::STALL_MIN_DUTY, now,
      lastLeftEncoderMotionAtMs, Threshold::STALL_WINDOW_MS);
  const bool rightStalled = Safety::sideHasStalled(
      appliedRight, Threshold::STALL_MIN_DUTY, now,
      lastRightEncoderMotionAtMs, Threshold::STALL_WINDOW_MS);
  if (leftStalled || rightStalled) {
    const int32_t stalledSides = (leftStalled ? 1 : 0) |
                                 (rightStalled ? 2 : 0);
    stallSuspected = true;
    armed = false;
    autoPhase = AutoPhase::IDLE;
    observationOnlyScan = false;
    activeBrake(true);
    sendEvent(Protocol::EventCode::STALL_SUSPECTED, 2, stalledSides);
  }
}

void serviceBuzzer() {
  const uint32_t now = millis();
  if (diagnosticBuzzerOffAtMs) {
    buzzerOn = true;
    digitalWrite(Pin::BUZZER, HIGH);
    return;
  }
  const bool alarm = waterContact || tiltStopped || stallSuspected ||
                     driveState == Protocol::DriveState::STUCK;
  if (!alarm) {
    buzzerOn = false;
    digitalWrite(Pin::BUZZER, LOW);
    return;
  }
  if (elapsed(now, lastBuzzerToggleAtMs, 200)) {
    lastBuzzerToggleAtMs = now;
    buzzerOn = !buzzerOn;
    digitalWrite(Pin::BUZZER, buzzerOn ? HIGH : LOW);
  }
  updateIndicators();
}

void sendScan(const ScanSample& sample) {
  Protocol::ScanPacket packet{};
  fillHeader(packet.header, Protocol::MessageType::SCAN);
  packet.scan_id = scanId;
  packet.angle_cdeg = sample.angle * 100;
  packet.distance_mm =
      sample.valid ? sample.mm : Protocol::UNKNOWN_DISTANCE_MM;
  packet.valid = sample.valid;
  packet.range_status = sample.status;
  // The VL53L0X API exposes RangeStatus, not a calibrated probability.
  // Never manufacture a confidence percentage from validity alone.
  packet.confidence_pct = Protocol::UNKNOWN_CONFIDENCE_PCT;
  sendPacket(packet);
}

bool chooseOpening() {
  float bestScore = -1.0f;
  int8_t bestIndex = -1;
  const float requiredWidth =
      calibration.chassis_width_mm +
      2.0f * Threshold::OPENING_MARGIN_EACH_SIDE_MM +
      Threshold::OPTICAL_UNCERTAINTY_MM;
  for (uint8_t i = 1; i + 1 < Threshold::SCAN_ANGLE_COUNT; ++i) {
    if (!scanSamples[i - 1].valid || !scanSamples[i].valid ||
        !scanSamples[i + 1].valid) continue;
    const uint16_t minDistance =
        min(scanSamples[i - 1].mm,
            min(scanSamples[i].mm, scanSamples[i + 1].mm));
    const float halfSpanRad =
        (scanSamples[i + 1].angle - scanSamples[i - 1].angle) *
        0.5f * PI / 180.0f;
    const float apparentWidth =
        2.0f * minDistance * tanf(halfSpanRad);
    if (apparentWidth < requiredWidth) continue;
    const float turnPenalty =
        fabsf(scanSamples[i].angle - calibration.servo_center_deg) * 2.0f;
    const float score = scanSamples[i].mm - turnPenalty;
    if (score > bestScore) {
      bestScore = score;
      bestIndex = i;
    }
  }
  if (bestIndex < 0) return false;
  int angleDelta = static_cast<int>(scanSamples[bestIndex].angle) -
                   calibration.servo_center_deg;
  if (!calibration.servo_low_is_left) angleDelta = -angleDelta;
  selectedTurnSign = angleDelta < 0 ? -1 : angleDelta > 0 ? 1 : 0;
  selectedTurnDurationMs = static_cast<uint32_t>(
      abs(angleDelta) * calibration.turn_90_ms / 90.0f);
  return true;
}

void beginAutoScan(bool observationOnly) {
  activeBrake(false);
  observationOnlyScan = observationOnly;
  driveState = Protocol::DriveState::AUTO_SCAN;
  autoPhase = AutoPhase::MOVE_SERVO;
  scanIndex = 0;
  ++scanId;
}

void serviceAuto() {
  if ((!armed && !observationOnlyScan) || autoPhase == AutoPhase::IDLE ||
      autoPhase == AutoPhase::STUCK) return;
  const uint32_t now = millis();
  switch (autoPhase) {
    case AutoPhase::ADVANCE:
      if (frontGate.snapshot(now).forward_blocked) beginAutoScan();
      else if (targetLeft != Threshold::AUTO_DUTY ||
               targetRight != Threshold::AUTO_DUTY || brakeLatched)
        requestMotion(Threshold::AUTO_DUTY, Threshold::AUTO_DUTY,
                      Protocol::DriveState::AUTO_ADVANCE);
      break;
    case AutoPhase::MOVE_SERVO:
      currentServoDeg = constrain(
          static_cast<int>(Threshold::SCAN_ANGLES_DEG[scanIndex]) +
              static_cast<int>(calibration.servo_center_deg) - 90,
          30, 150);
      writeScanServo(currentServoDeg);
      autoPhaseAtMs = now;
      autoPhase = AutoPhase::WAIT_SERVO;
      break;
    case AutoPhase::WAIT_SERVO:
      if (elapsed(now, autoPhaseAtMs, Threshold::SERVO_SETTLE_MS))
        autoPhase = AutoPhase::MEASURE;
      break;
    case AutoPhase::MEASURE: {
      VL53L0X_RangingMeasurementData_t measurement{};
      tof.rangingTest(&measurement, false);
      ScanSample& sample = scanSamples[scanIndex];
      sample.angle = currentServoDeg;
      sample.mm = measurement.RangeMilliMeter;
      sample.status = measurement.RangeStatus;
      sample.valid = measurement.RangeStatus == 0 &&
                     sample.mm >= Threshold::TOF_MIN_MM &&
                     sample.mm <= Threshold::TOF_MAX_MM;
      if (sample.valid) lastTofValidAtMs = now;
      const uint8_t center = calibration.servo_center_deg
                                 ? calibration.servo_center_deg
                                 : Threshold::SERVO_CENTER_DEG;
      if (abs(static_cast<int>(currentServoDeg) - center) <= 2) {
        latestCenterTofMm = sample.valid
                                ? sample.mm
                                : Protocol::UNKNOWN_DISTANCE_MM;
        latestCenterTofStatus = sample.status;
        lastCenterTofAtMs = now;
      }
      sendScan(sample);
      if (++scanIndex < Threshold::SCAN_ANGLE_COUNT) {
        autoPhase = AutoPhase::MOVE_SERVO;
      } else if (observationOnlyScan) {
        currentServoDeg = calibration.servo_center_deg
                              ? calibration.servo_center_deg
                              : Threshold::SERVO_CENTER_DEG;
        writeScanServo(currentServoDeg);
        autoPhaseAtMs = now;
        autoPhase = AutoPhase::CENTER_WAIT;
      } else if (!chooseOpening()) {
        activeBrake(false);
        driveState = Protocol::DriveState::STUCK;
        autoPhase = AutoPhase::STUCK;
        sendEvent(Protocol::EventCode::AUTO_STUCK, 2);
      } else {
        currentServoDeg = calibration.servo_center_deg;
        writeScanServo(currentServoDeg);
        if (!selectedTurnSign || !selectedTurnDurationMs) {
          autoPhaseAtMs = now;
          autoPhase = AutoPhase::CENTER_WAIT;
        } else {
          const int16_t left =
              selectedTurnSign < 0 ? -Threshold::TURN_DUTY
                                   : Threshold::TURN_DUTY;
          requestMotion(left, -left, Protocol::DriveState::AUTO_TURN);
          autoPhaseAtMs = now;
          autoPhase = AutoPhase::TURN;
        }
      }
      break;
    }
    case AutoPhase::TURN:
      if (elapsed(now, autoPhaseAtMs, selectedTurnDurationMs)) {
        activeBrake(false);
        autoPhaseAtMs = now;
        autoPhase = AutoPhase::CENTER_WAIT;
        sendEvent(
            Protocol::EventCode::AUTO_TURN_COMPLETE, 0,
            selectedTurnSign *
                static_cast<int32_t>(selectedTurnDurationMs));
      }
      break;
    case AutoPhase::CENTER_WAIT:
      if (elapsed(now, autoPhaseAtMs, Threshold::SERVO_SETTLE_MS)) {
        if (observationOnlyScan) {
          observationOnlyScan = false;
          lastScanCompletedAtMs = now;
          autoPhase = AutoPhase::IDLE;
          // Remain actively braked but return command ownership to manual
          // mode so the operator may turn or reverse away from the obstacle.
          driveState = armed ? Protocol::DriveState::MANUAL
                             : Protocol::DriveState::STOPPED;
        } else {
          autoPhase = AutoPhase::VERIFY;
        }
      }
      break;
    case AutoPhase::VERIFY:
      sampleFront(true);
      if (!frontGate.snapshot(millis()).forward_blocked) {
        autoPhase = AutoPhase::ADVANCE;
        driveState = Protocol::DriveState::AUTO_ADVANCE;
      } else {
        activeBrake(false);
        driveState = Protocol::DriveState::STUCK;
        autoPhase = AutoPhase::STUCK;
        sendEvent(Protocol::EventCode::AUTO_STUCK, 2);
      }
      break;
    default: break;
  }
}

void processCommand(const Protocol::CommandPacket& packet,
                    uint32_t receivedAtMs) {
  const auto command =
      static_cast<Protocol::DriveCommand>(packet.command);
  if (!packet.ttl_ms ||
      packet.ttl_ms > Protocol::MAX_COMMAND_TTL_MS ||
      elapsed(millis(), receivedAtMs, packet.ttl_ms)) {
    sendAck(packet.header.sequence, Protocol::AckStatus::REJECTED,
            Protocol::AckReason::BAD_TTL);
    return;
  }
  if (command == Protocol::DriveCommand::STOP) {
    activeSession = packet.header.session_id;
    lastCommandSequence = packet.header.sequence;
    armed = packet.mode == 1 && !waterContact && !tiltStopped &&
            !stallSuspected && driveState != Protocol::DriveState::STUCK;
    lastValidCommandAtMs = receivedAtMs;
    acceptedCommandTtlMs = packet.ttl_ms;
    autoPhase = AutoPhase::IDLE;
    observationOnlyScan = false;
    activeBrake(false);
    if (armed) driveState = Protocol::DriveState::MANUAL;
    sendAck(packet.header.sequence, Protocol::AckStatus::ACCEPTED,
            Protocol::AckReason::NONE);
    return;
  }
  if (packet.header.session_id != activeSession) {
    activeSession = packet.header.session_id;
    lastCommandSequence = packet.header.sequence;
    armed = false;
    autoPhase = AutoPhase::IDLE;
    observationOnlyScan = false;
    activeBrake(false);
    sendAck(packet.header.sequence, Protocol::AckStatus::REJECTED,
            Protocol::AckReason::BAD_SESSION);
    return;
  }
  if (lastCommandSequence &&
      static_cast<int32_t>(packet.header.sequence -
                           lastCommandSequence) <= 0) {
    sendAck(packet.header.sequence, Protocol::AckStatus::DUPLICATE,
            Protocol::AckReason::BAD_SEQUENCE);
    return;
  }
  lastCommandSequence = packet.header.sequence;
  lastValidCommandAtMs = receivedAtMs;
  acceptedCommandTtlMs = packet.ttl_ms;

  if (command == Protocol::DriveCommand::MANUAL && packet.mode == 1) {
    if (waterContact || tiltStopped || gyroCalibrationActive) {
      sendAck(packet.header.sequence, Protocol::AckStatus::REJECTED,
              Protocol::AckReason::SENSOR_UNAVAILABLE);
      return;
    }
    const bool modeChanged =
        driveState != Protocol::DriveState::MANUAL;
    armed = true;
    stallSuspected = false;
    autoPhase = AutoPhase::IDLE;
    observationOnlyScan = false;
    if (modeChanged) activeBrake(false);
    driveState = Protocol::DriveState::MANUAL;
  } else if (command == Protocol::DriveCommand::AUTO &&
             packet.mode == 1) {
    if (!autoCalibrated()) {
      sendAck(packet.header.sequence, Protocol::AckStatus::REJECTED,
              Protocol::AckReason::CALIBRATION_REQUIRED);
      return;
    }
    if (!tofReady || !servoReady || gyroCalibrationActive) {
      sendAck(packet.header.sequence, Protocol::AckStatus::REJECTED,
              Protocol::AckReason::SENSOR_UNAVAILABLE);
      return;
    }
    if (waterContact || tiltStopped || stallSuspected) {
      sendAck(packet.header.sequence, Protocol::AckStatus::REJECTED,
              Protocol::AckReason::SENSOR_UNAVAILABLE);
      return;
    }
    armed = true;
    stallSuspected = false;
    if (autoPhase == AutoPhase::IDLE) {
      sampleFront(true);
      if (frontGate.snapshot(millis()).forward_blocked)
        beginAutoScan();
      else
        autoPhase = AutoPhase::ADVANCE;
      sendEvent(Protocol::EventCode::AUTO_STARTED, 0);
    }
  } else if (command == Protocol::DriveCommand::DRIVE) {
    if (!armed || driveState != Protocol::DriveState::MANUAL) {
      sendAck(packet.header.sequence, Protocol::AckStatus::REJECTED,
              Protocol::AckReason::DISARMED);
      return;
    }
    const int16_t left =
        constrain(packet.left_percent, -100, 100) * 255 / 100;
    const int16_t right =
        constrain(packet.right_percent, -100, 100) * 255 / 100;
    if (!requestMotion(left, right,
                       Protocol::DriveState::MANUAL)) {
      sendAck(packet.header.sequence, Protocol::AckStatus::REJECTED,
              Protocol::AckReason::FRONT_BLOCKED);
      return;
    }
  } else {
    sendAck(packet.header.sequence, Protocol::AckStatus::REJECTED,
            Protocol::AckReason::DISARMED);
    return;
  }
  sendAck(packet.header.sequence, Protocol::AckStatus::ACCEPTED,
          Protocol::AckReason::NONE);
}

void drainCommand() {
  Protocol::CommandPacket packet{};
  uint32_t receivedAt = 0;
  portENTER_CRITICAL(&radioMux);
  if (commandPending) {
    packet = pendingCommand;
    receivedAt = pendingCommandReceivedAtMs;
    commandPending = false;
  }
  portEXIT_CRITICAL(&radioMux);
  if (receivedAt) processCommand(packet, receivedAt);
}

uint16_t buildStatusFlags() {
  uint16_t flags = 0;
  const auto front = frontGate.snapshot(millis());
  if (front.latest_sample == Safety::RangeValidity::VALID)
    flags |= Protocol::FRONT_VALID;
  if (front.freshness == Safety::RangeFreshness::FRESH)
    flags |= Protocol::FRONT_FRESH;
  if (front.forward_blocked) flags |= Protocol::FRONT_BLOCKED;
  if (lastTofValidAtMs && !elapsed(millis(), lastTofValidAtMs, 1000))
    flags |= Protocol::TOF_VALID;
  if (imuSampleValid && !elapsed(millis(), lastImuAtMs, 500))
    flags |= Protocol::IMU_VALID;
  if (lastDhtValidAtMs &&
      !elapsed(millis(), lastDhtValidAtMs, 6000))
    flags |= Protocol::DHT_VALID;
  if (calibration.gas_baseline) flags |= Protocol::GAS_BASELINED;
  if (waterCalibrated()) flags |= Protocol::WATER_CALIBRATED;
  if (waterContact) flags |= Protocol::WATER_CONTACT;
  if (lastValidCommandAtMs && acceptedCommandTtlMs &&
      !elapsed(millis(), lastValidCommandAtMs, acceptedCommandTtlMs))
    flags |= Protocol::LINK_FRESH;
  if (radioReady) flags |= Protocol::RADIO_CONFIGURED;
  if (armed) flags |= Protocol::COMMAND_ARMED;
  if (!observationOnlyScan && autoPhase != AutoPhase::IDLE &&
      autoPhase != AutoPhase::STUCK)
    flags |= Protocol::AUTO_ENABLED;
  if (stallSuspected) flags |= Protocol::STALL_SUSPECTED |
                               Protocol::ODOMETRY_DEGRADED;
  if (!autoCalibrated() || !waterCalibrated())
    flags |= Protocol::CALIBRATION_INCOMPLETE;
  return flags;
}

bool i2cAddressPresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

uint16_t diagnosticFlags() {
  uint16_t flags = 0;
  const auto front = frontGate.snapshot(millis());
  if (imuReady) flags |= Protocol::DIAG_IMU_READY;
  if (tofReady) flags |= Protocol::DIAG_TOF_READY;
  if (servoReady) flags |= Protocol::DIAG_SERVO_READY;
  if (lastDhtValidAtMs && !elapsed(millis(), lastDhtValidAtMs, 6000))
    flags |= Protocol::DIAG_DHT_READY;
  if (front.has_valid_distance) flags |= Protocol::DIAG_FRONT_VALID;
  if (radioReady) flags |= Protocol::DIAG_RADIO_READY;
  if (digitalRead(Pin::LEFT_ENCODER))
    flags |= Protocol::DIAG_LEFT_ENCODER_HIGH;
  if (digitalRead(Pin::RIGHT_ENCODER))
    flags |= Protocol::DIAG_RIGHT_ENCODER_HIGH;
  if (autoCalibrated()) flags |= Protocol::DIAG_CALIBRATION_READY;
  if (armed) flags |= Protocol::DIAG_ARMED;
  if (brakeLatched) flags |= Protocol::DIAG_BRAKE_LATCHED;
  if (calibration.motor_direction_known &&
      calibration.micrometers_per_tick)
    flags |= Protocol::DIAG_STALL_MONITOR_READY;
  return flags;
}

void sendDiagnosticResult(const Protocol::DiagnosticCommandPacket& request,
                          Protocol::DiagnosticStatus status, uint16_t flags,
                          int32_t a = 0, int32_t b = 0, int32_t c = 0,
                          int32_t d = 0) {
  Protocol::DiagnosticResultPacket result{};
  fillHeader(result.header, Protocol::MessageType::DIAGNOSTIC_RESULT);
  result.request_sequence = request.header.sequence;
  result.action = request.action;
  result.status = static_cast<uint8_t>(status);
  result.flags = flags;
  result.value_a = a;
  result.value_b = b;
  result.value_c = c;
  result.value_d = d;
  sendPacket(result);
}

void processDiagnostic(const Protocol::DiagnosticCommandPacket& request) {
  const auto action =
      static_cast<Protocol::DiagnosticAction>(request.action);
  activeSession = request.header.session_id;
  Protocol::DiagnosticStatus status = Protocol::DiagnosticStatus::OK;
  uint16_t flags = diagnosticFlags();
  int32_t a = 0, b = 0, c = 0, d = 0;
  switch (action) {
    case Protocol::DiagnosticAction::STATUS: {
      const auto front = frontGate.snapshot(millis());
      a = buildStatusFlags();
      b = front.has_valid_distance
              ? static_cast<int32_t>(front.last_valid_cm * 10.0f)
              : -1;
      c = leftTicks;
      d = rightTicks;
      break;
    }
    case Protocol::DiagnosticAction::I2C_SCAN: {
      uint8_t count = 0;
      for (uint8_t address = 1; address < 127; ++address) {
        if (!i2cAddressPresent(address)) continue;
        ++count;
        if (address == Pin::VL53L0X_ADDRESS)
          flags |= Protocol::DIAG_I2C_TOF_FOUND;
        if (address == Pin::MPU6050_ADDRESS)
          flags |= Protocol::DIAG_I2C_IMU_FOUND;
      }
      a = count;
      status = count ? Protocol::DiagnosticStatus::OK
                     : Protocol::DiagnosticStatus::FAILED;
      break;
    }
    case Protocol::DiagnosticAction::DHT: {
      const float humidity = dht.readHumidity();
      const float temperature = dht.readTemperature();
      if (isfinite(humidity) && isfinite(temperature)) {
        humidityPct = humidity;
        temperatureC = temperature;
        lastDhtValidAtMs = millis();
        flags |= Protocol::DIAG_DHT_READY;
        a = static_cast<int32_t>(temperature * 100.0f);
        b = static_cast<int32_t>(humidity * 100.0f);
      } else {
        status = Protocol::DiagnosticStatus::FAILED;
      }
      break;
    }
    case Protocol::DiagnosticAction::IMU:
      serviceImu();
      if (imuReady && imuSampleValid) {
        a = static_cast<int32_t>(pitchDeg * 100.0f);
        b = static_cast<int32_t>(rollDeg * 100.0f);
        c = static_cast<int32_t>(headingDeg * 100.0f);
      } else status = Protocol::DiagnosticStatus::FAILED;
      break;
    case Protocol::DiagnosticAction::GAS:
      a = gasRaw;
      b = gasPinMv;
      c = calibration.gas_baseline;
      status = calibration.gas_baseline
                   ? Protocol::DiagnosticStatus::OK
                   : Protocol::DiagnosticStatus::WARNING;
      break;
    case Protocol::DiagnosticAction::WATER:
      a = waterRaw;
      b = waterPinMv;
      c = calibration.water_dry;
      d = calibration.water_wet;
      status = waterCalibrated() ? Protocol::DiagnosticStatus::OK
                                 : Protocol::DiagnosticStatus::WARNING;
      break;
    case Protocol::DiagnosticAction::FRONT_RANGE: {
      const UltrasonicSample sonar = readFrontUltrasonic();
      frontGate.observe(sonar.valid, sonar.cm, millis());
      const TofSample optical = readCenteredTof();
      a = sonar.valid ? static_cast<int32_t>(sonar.cm * 10.0f) : -1;
      b = optical.valid ? optical.mm : -1;
      if (sonar.valid && optical.valid) c = min<int32_t>(a, b);
      else if (sonar.valid) c = a;
      else if (optical.valid) c = b;
      else c = -1;
      d = c >= 0 && c <= static_cast<int32_t>(Threshold::FRONT_STOP_CM * 10.0f);
      status = sonar.valid && optical.valid && optical.status == 0
                   ? Protocol::DiagnosticStatus::OK
                   : c >= 0 ? Protocol::DiagnosticStatus::WARNING
                            : Protocol::DiagnosticStatus::FAILED;
      updateIndicators();
      break;
    }
    case Protocol::DiagnosticAction::TOF: {
      const TofSample optical = readCenteredTof();
      a = optical.valid ? optical.mm : -1;
      b = optical.status;
      c = currentServoDeg;
      status = optical.valid
                   ? optical.status == 0 ? Protocol::DiagnosticStatus::OK
                                         : Protocol::DiagnosticStatus::WARNING
                   : Protocol::DiagnosticStatus::FAILED;
      break;
    }
    case Protocol::DiagnosticAction::ENCODERS:
      a = leftTicks;
      b = rightTicks;
      c = digitalRead(Pin::LEFT_ENCODER);
      d = digitalRead(Pin::RIGHT_ENCODER);
      status = calibration.micrometers_per_tick
                   ? Protocol::DiagnosticStatus::OK
                   : Protocol::DiagnosticStatus::WARNING;
      break;
    case Protocol::DiagnosticAction::START_SCAN:
      if (armed || !servoReady || !tofReady || autoPhase != AutoPhase::IDLE) {
        status = Protocol::DiagnosticStatus::REJECTED;
      } else {
        beginAutoScan(true);
        a = scanId;
      }
      break;
    case Protocol::DiagnosticAction::SERVO:
      if (armed || !servoReady) {
        status = Protocol::DiagnosticStatus::REJECTED;
      } else {
        writeScanServo(constrain(request.argument, 30, 150));
        delay(25);
        a = currentServoDeg;
        b = currentServoDeg;
        c = pulseIn(Pin::SERVO, HIGH, 30000UL);
        if (c < 400 || c > 2600)
          status = Protocol::DiagnosticStatus::FAILED;
      }
      break;
    case Protocol::DiagnosticAction::LEDS:
      if (armed) {
        status = Protocol::DiagnosticStatus::REJECTED;
      } else {
        digitalWrite(Pin::RED, HIGH);
        digitalWrite(Pin::GREEN, HIGH);
        diagnosticLedsOffAtMs = millis() + 500;
      }
      break;
    case Protocol::DiagnosticAction::BUZZER:
      if (armed) {
        status = Protocol::DiagnosticStatus::REJECTED;
      } else {
        digitalWrite(Pin::BUZZER, request.argument ? HIGH : LOW);
        buzzerOn = request.argument != 0;
        diagnosticBuzzerOffAtMs = request.argument ? millis() + 500 : 0;
        a = request.argument ? 1 : 0;
      }
      break;
    case Protocol::DiagnosticAction::RADIO_LINK:
      a = lastGatewayRssiDbm == Protocol::RSSI_UNAVAILABLE_DBM
              ? -999 : lastGatewayRssiDbm;
      b = lastValidCommandAtMs
              ? static_cast<int32_t>(millis() - lastValidCommandAtMs) : -1;
      c = radioReady ? 1 : 0;
      d = static_cast<int32_t>(activeSession);
      status = radioReady &&
                       lastGatewayRssiDbm != Protocol::RSSI_UNAVAILABLE_DBM
                   ? Protocol::DiagnosticStatus::OK
                   : Protocol::DiagnosticStatus::WARNING;
      break;
    case Protocol::DiagnosticAction::MOTOR_STATE:
      a = targetLeft;
      b = targetRight;
      c = appliedLeft;
      d = appliedRight;
      status = motorPwmReady ? Protocol::DiagnosticStatus::OK
                             : Protocol::DiagnosticStatus::FAILED;
      break;
    case Protocol::DiagnosticAction::SENSOR_FRESHNESS: {
      const uint32_t now = millis();
      a = lastFrontAtMs ? static_cast<int32_t>(now - lastFrontAtMs) : -1;
      b = lastDhtValidAtMs
              ? static_cast<int32_t>(now - lastDhtValidAtMs) : -1;
      c = lastImuAtMs ? static_cast<int32_t>(now - lastImuAtMs) : -1;
      d = lastCenterTofAtMs
              ? static_cast<int32_t>(now - lastCenterTofAtMs) : -1;
      status = Protocol::DiagnosticStatus::OK;
      break;
    }
    case Protocol::DiagnosticAction::CALIBRATION_STATUS:
      a = calibration.chassis_width_mm;
      b = calibration.track_width_mm;
      c = calibration.micrometers_per_tick;
      d = calibration.turn_90_ms;
      status = autoCalibrated() ? Protocol::DiagnosticStatus::OK
                                : Protocol::DiagnosticStatus::WARNING;
      break;
    case Protocol::DiagnosticAction::CALIBRATE_IMU: {
      const uint32_t now = millis();
      serviceImu();
      if (armed || !imuReady || !imuSampleValid ||
          elapsed(now, lastImuAtMs, 500) ||
          !isfinite(rawPitchDeg) || !isfinite(rawRollDeg)) {
        status = Protocol::DiagnosticStatus::REJECTED;
        break;
      }
      calibration.pitch_offset_cdeg =
          static_cast<int16_t>(roundf(rawPitchDeg * 100.0f));
      calibration.roll_offset_cdeg =
          static_cast<int16_t>(roundf(rawRollDeg * 100.0f));
      calibration.imu_level_known = true;
      preferences.putShort("pitch_off", calibration.pitch_offset_cdeg);
      preferences.putShort("roll_off", calibration.roll_offset_cdeg);
      preferences.putBool("imu_level", true);
      pitchDeg = 0.0f;
      rollDeg = 0.0f;
      gyroCalibrationSum = 0.0;
      gyroCalibrationSumSquares = 0.0;
      gyroCalibrationSamples = 0;
      gyroCalibrationStartedAtMs = now;
      gyroCalibrationActive = true;
      headingDeg = 0.0f;
      a = Threshold::GYRO_BIAS_CALIBRATION_MS;
      b = Threshold::GYRO_BIAS_MIN_SAMPLES;
      break;
    }
    default:
      status = Protocol::DiagnosticStatus::REJECTED;
      break;
  }
  sendDiagnosticResult(request, status, flags, a, b, c, d);
}

void drainDiagnostic() {
  Protocol::DiagnosticCommandPacket request{};
  bool hasRequest = false;
  portENTER_CRITICAL(&radioMux);
  if (diagnosticPending) {
    request = pendingDiagnostic;
    diagnosticPending = false;
    hasRequest = true;
  }
  portEXIT_CRITICAL(&radioMux);
  if (hasRequest) processDiagnostic(request);
}

void serviceDiagnosticOutputs() {
  const uint32_t now = millis();
  if (diagnosticLedsOffAtMs &&
      static_cast<int32_t>(now - diagnosticLedsOffAtMs) >= 0) {
    diagnosticLedsOffAtMs = 0;
    digitalWrite(Pin::RED, LOW);
    digitalWrite(Pin::GREEN, LOW);
  }
  if (diagnosticBuzzerOffAtMs &&
      static_cast<int32_t>(now - diagnosticBuzzerOffAtMs) >= 0) {
    diagnosticBuzzerOffAtMs = 0;
    buzzerOn = false;
    digitalWrite(Pin::BUZZER, LOW);
  }
}

void sendTelemetry() {
  const uint32_t now = millis();
  if (!elapsed(now, lastTelemetryAtMs,
               Protocol::TELEMETRY_INTERVAL_MS)) return;
  lastTelemetryAtMs = now;
  Protocol::TelemetryPacket packet{};
  fillHeader(packet.header, Protocol::MessageType::TELEMETRY);
  packet.left_ticks = leftTicks;
  packet.right_ticks = rightTicks;
  packet.heading_cdeg =
      imuSampleValid ? static_cast<uint16_t>(headingDeg * 100.0f) : 0;
  packet.pitch_cdeg =
      imuSampleValid ? static_cast<int16_t>(pitchDeg * 100.0f) : 0;
  packet.roll_cdeg =
      imuSampleValid ? static_cast<int16_t>(rollDeg * 100.0f) : 0;
  const auto front = frontGate.snapshot(now);
  packet.front_mm =
      front.has_valid_distance &&
              front.freshness == Safety::RangeFreshness::FRESH
          ? static_cast<uint16_t>(front.last_valid_cm * 10.0f)
          : Protocol::UNKNOWN_DISTANCE_MM;
  packet.tof_mm = lastCenterTofAtMs &&
                          !elapsed(now, lastCenterTofAtMs, 1000) &&
                          latestCenterTofMm != Protocol::UNKNOWN_DISTANCE_MM
                      ? latestCenterTofMm
                      : Protocol::UNKNOWN_DISTANCE_MM;
  packet.gas_raw = gasRaw;
  packet.gas_pin_mv = gasPinMv;
  packet.water_raw = waterRaw;
  packet.water_pin_mv = waterPinMv;
  packet.chassis_width_mm = calibration.chassis_width_mm;
  packet.track_width_mm = calibration.track_width_mm;
  packet.micrometers_per_tick = calibration.micrometers_per_tick;
  packet.temperature_centi_c =
      lastDhtValidAtMs
          ? static_cast<int16_t>(temperatureC * 100.0f)
          : 0;
  packet.humidity_centi_pct =
      lastDhtValidAtMs
          ? static_cast<uint16_t>(humidityPct * 100.0f)
          : 0;
  packet.status_flags = buildStatusFlags();
  packet.servo_deg = currentServoDeg;
  packet.drive_state = static_cast<uint8_t>(driveState);
  packet.last_rssi_dbm = Protocol::RSSI_UNAVAILABLE_DBM;
  packet.reset_reason = static_cast<uint8_t>(esp_reset_reason());
  if (!calibration.gas_baseline)
    packet.gas_state = static_cast<uint8_t>(
        elapsed(now, 0, Threshold::GAS_BOOT_STABILIZE_MS)
            ? Protocol::SensorState::RAW_ONLY
            : Protocol::SensorState::WARMING);
  else
    packet.gas_state = static_cast<uint8_t>(
        gasAdvisory ? Protocol::SensorState::QUALITATIVE_ADVISORY
                    : Protocol::SensorState::QUALITATIVE_READY);
  packet.water_state = static_cast<uint8_t>(
      !waterCalibrated()
          ? Protocol::SensorState::RAW_ONLY
          : waterContact ? Protocol::SensorState::CONTACT
                         : Protocol::SensorState::DRY_BASELINE);
  sendPacket(packet);
}

void loadCalibration() {
  preferences.begin("deeptrack", false);
  calibration.chassis_width_mm =
      preferences.getUShort("width_mm", 0);
  calibration.turn_90_ms =
      preferences.getUShort("turn90_ms", 0);
  calibration.water_dry =
      preferences.getUShort("water_dry", 0);
  calibration.water_wet =
      preferences.getUShort("water_wet", 0);
  calibration.gas_baseline =
      preferences.getUShort("gas_base", 0);
  calibration.track_width_mm =
      preferences.getUShort("track_mm", 0);
  calibration.micrometers_per_tick =
      preferences.getUShort("tick_um", 0);
  calibration.pitch_offset_cdeg = preferences.getShort("pitch_off", 0);
  calibration.roll_offset_cdeg = preferences.getShort("roll_off", 0);
  calibration.gyro_z_bias_dps = preferences.getFloat("gyro_bias", 0.0f);
  calibration.servo_center_deg = preferences.getUChar("servo_ctr", 0);
  calibration.servo_direction_known = preferences.isKey("servo_left");
  calibration.servo_low_is_left = preferences.getBool("servo_left", false);
  calibration.motor_direction_known = preferences.isKey("motor_known");
  calibration.imu_level_known = preferences.getBool("imu_level", false);
  calibration.gyro_bias_known = preferences.getBool("gyro_known", false);
  leftInverted = preferences.getBool("left_inv", true);
  rightInverted = preferences.getBool("right_inv", true);
}

void printStatus() {
  const auto front = frontGate.snapshot(millis());
  Serial.printf(
      "MISSION armed=%u radio=%u drive_state=%u front=%s",
      armed, radioReady, static_cast<unsigned>(driveState),
      front.forward_blocked ? "BLOCKED" : "CLEAR");
  if (front.has_valid_distance)
    Serial.printf(" %.1fcm", front.last_valid_cm);
  Serial.println();
  Serial.printf(
      "SENSORS imu=%u tof=%u servo=%u gas_raw=%u water_raw=%u\n",
      imuReady, tofReady, servoReady, gasRaw, waterRaw);
  Serial.printf(
      "CAL width_mm=%u track_mm=%u tick_um=%u turn90_ms=%u water_dry=%u water_wet=%u gas_base=%u auto_ready=%u\n",
      calibration.chassis_width_mm, calibration.track_width_mm,
      calibration.micrometers_per_tick, calibration.turn_90_ms,
      calibration.water_dry, calibration.water_wet,
      calibration.gas_baseline, autoCalibrated());
  Serial.printf("DIRECTION motor_known=%u left_inv=%u right_inv=%u servo_known=%u center=%u low_is_left=%u\n",
                calibration.motor_direction_known, leftInverted,
                rightInverted, calibration.servo_direction_known,
                calibration.servo_center_deg,
                calibration.servo_low_is_left);
  Serial.printf(
      "IMU_CAL level_known=%u pitch_offset=%.2f roll_offset=%.2f gyro_bias_known=%u gyro_z_bias=%.4f calibrating=%u\n",
      calibration.imu_level_known, calibration.pitch_offset_cdeg / 100.0f,
      calibration.roll_offset_cdeg / 100.0f,
      calibration.gyro_bias_known, calibration.gyro_z_bias_dps,
      gyroCalibrationActive);
}

void handleSerialCommand(String command) {
  command.trim();
  command.toLowerCase();
  if (!command.length()) return;
  const bool benchMotionCommand = command == "fwd" || command == "rev" ||
                                  command == "left" || command == "right";
  if (benchMotionCommand && armed) {
    Serial.println("REJECTED bench motion requires radio session disarmed");
    return;
  }
  if (command.startsWith("cal ") && command != "cal clear" &&
      (armed || !brakeLatched || benchMotionUntilMs)) {
    armed = false;
    autoPhase = AutoPhase::IDLE;
    observationOnlyScan = false;
    activeBrake(false);
    Serial.println(
        "REJECTED calibration requires disarmed active brake; stopped now, run command again");
    return;
  }
  if (command == "status") printStatus();
  else if (command == "front") {
    sampleFront(true);
    printStatus();
  } else if (command == "stop" || command == "brake") {
    activeBrake(false);
  } else if (command == "fwd") {
    if (requestMotion(Threshold::MANUAL_DUTY,
                      Threshold::MANUAL_DUTY,
                      Protocol::DriveState::MANUAL))
      benchMotionUntilMs =
          millis() + Threshold::SERIAL_BENCH_BURST_MS;
  } else if (command == "rev") {
    Serial.println(
        "CAUTION rear clearance is unsensed; bounded bench burst only");
    if (requestMotion(-Threshold::MANUAL_DUTY,
                      -Threshold::MANUAL_DUTY,
                      Protocol::DriveState::MANUAL))
      benchMotionUntilMs =
          millis() + Threshold::SERIAL_BENCH_BURST_MS;
  } else if (command == "left" || command == "right") {
    const int sign = command == "left" ? -1 : 1;
    if (requestMotion(sign * Threshold::MANUAL_DUTY,
                      -sign * Threshold::MANUAL_DUTY,
                      Protocol::DriveState::MANUAL))
      benchMotionUntilMs =
          millis() + Threshold::SERIAL_BENCH_BURST_MS;
  } else if (command.startsWith("cal width ")) {
    const int value = command.substring(10).toInt();
    if (value >= 120 && value <= 600) {
      calibration.chassis_width_mm = value;
      preferences.putUShort("width_mm", value);
      sendEvent(Protocol::EventCode::CALIBRATION_CHANGED, 0,
                value);
    } else {
      Serial.println(
          "REJECTED width must be measured millimetres 120..600");
    }
  } else if (command.startsWith("cal turn90 ")) {
    const int value = command.substring(11).toInt();
    if (value >= 200 && value <= 1000) {
      calibration.turn_90_ms = value;
      preferences.putUShort("turn90_ms", value);
      sendEvent(Protocol::EventCode::CALIBRATION_CHANGED, 0,
                value);
    } else {
      Serial.println(
          "REJECTED turn90 must be measured milliseconds 200..1000");
    }
  } else if (command.startsWith("cal track ")) {
    const int value = command.substring(10).toInt();
    if (value >= 100 && value <= 600) {
      calibration.track_width_mm = value;
      preferences.putUShort("track_mm", value);
      Serial.printf("RECORDED driven-wheel track width=%dmm\n", value);
    } else {
      Serial.println("REJECTED track must be measured millimetres 100..600");
    }
  } else if (command.startsWith("cal tickum ")) {
    const int value = command.substring(11).toInt();
    if (value >= 100 && value <= 60000) {
      calibration.micrometers_per_tick = value;
      preferences.putUShort("tick_um", value);
      Serial.printf("RECORDED distance per encoder tick=%d micrometres\n", value);
    } else {
      Serial.println("REJECTED tickum must be measured micrometres 100..60000");
    }
  } else if (command.startsWith("cal motors ")) {
    int left = -1, right = -1;
    if (sscanf(command.c_str(), "cal motors %d %d", &left, &right) == 2 &&
        (left == 0 || left == 1) && (right == 0 || right == 1)) {
      leftInverted = left == 1;
      rightInverted = right == 1;
      calibration.motor_direction_known = true;
      preferences.putBool("left_inv", leftInverted);
      preferences.putBool("right_inv", rightInverted);
      preferences.putBool("motor_known", true);
      Serial.printf("RECORDED motor inversion left=%u right=%u\n",
                    leftInverted, rightInverted);
    } else {
      Serial.println("REJECTED use cal motors <left 0|1> <right 0|1>");
    }
  } else if (command.startsWith("cal servo ")) {
    int center = 0;
    char direction[8]{};
    if (sscanf(command.c_str(), "cal servo %d %7s", &center, direction) == 2 &&
        center >= 80 && center <= 100 &&
        (strcmp(direction, "left") == 0 || strcmp(direction, "right") == 0)) {
      calibration.servo_center_deg = center;
      calibration.servo_low_is_left = strcmp(direction, "left") == 0;
      calibration.servo_direction_known = true;
      preferences.putUChar("servo_ctr", center);
      preferences.putBool("servo_left", calibration.servo_low_is_left);
      currentServoDeg = center;
      if (servoReady) writeScanServo(center);
      Serial.printf("RECORDED servo center=%d low_angles_point=%s\n",
                    center, direction);
    } else {
      Serial.println("REJECTED use cal servo <center 80..100> <left|right>");
    }
  } else if (command == "cal waterdry") {
    calibration.water_dry = waterRaw;
    preferences.putUShort("water_dry", waterRaw);
    Serial.printf("RECORDED water dry=%u; wet point still required\n",
                  waterRaw);
  } else if (command == "cal waterwet") {
    calibration.water_wet = waterRaw;
    preferences.putUShort("water_wet", waterRaw);
    Serial.printf("RECORDED water wet=%u calibrated=%u\n", waterRaw,
                  waterCalibrated());
  } else if (command == "cal gasbase") {
    if (gasWindowCount == Threshold::GAS_MEDIAN_WINDOW &&
        !isnan(gasEma) &&
        elapsed(millis(), 0, Threshold::GAS_BOOT_STABILIZE_MS)) {
      calibration.gas_baseline =
          static_cast<uint16_t>(gasEma);
      preferences.putUShort("gas_base",
                            calibration.gas_baseline);
      Serial.println(
          "RECORDED qualitative gas baseline; never interpreted as ppm/%LEL");
    } else {
      Serial.println(
          "REJECTED gas baseline requires completed boot stabilization and a full filter window");
    }
  } else if (command == "cal imulevel") {
    if (imuReady && imuSampleValid &&
        !elapsed(millis(), lastImuAtMs, 500) &&
        isfinite(rawPitchDeg) && isfinite(rawRollDeg)) {
      calibration.pitch_offset_cdeg =
          static_cast<int16_t>(roundf(rawPitchDeg * 100.0f));
      calibration.roll_offset_cdeg =
          static_cast<int16_t>(roundf(rawRollDeg * 100.0f));
      calibration.imu_level_known = true;
      preferences.putShort("pitch_off", calibration.pitch_offset_cdeg);
      preferences.putShort("roll_off", calibration.roll_offset_cdeg);
      preferences.putBool("imu_level", true);
      pitchDeg = rawPitchDeg - calibration.pitch_offset_cdeg / 100.0f;
      rollDeg = rawRollDeg - calibration.roll_offset_cdeg / 100.0f;
      Serial.printf("RECORDED IMU level offsets pitch=%.2f roll=%.2f deg\n",
                    calibration.pitch_offset_cdeg / 100.0f,
                    calibration.roll_offset_cdeg / 100.0f);
    } else {
      Serial.println("REJECTED IMU level requires a fresh valid sample");
    }
  } else if (command == "cal gyrobias") {
    if (imuReady && imuSampleValid &&
        !elapsed(millis(), lastImuAtMs, 500)) {
      gyroCalibrationSum = 0.0;
      gyroCalibrationSumSquares = 0.0;
      gyroCalibrationSamples = 0;
      gyroCalibrationStartedAtMs = millis();
      gyroCalibrationActive = true;
      headingDeg = 0.0f;
      Serial.println(
          "GYRO CALIBRATION STARTED: keep rover stationary for 3 seconds");
    } else {
      Serial.println("REJECTED gyro bias requires a fresh valid IMU sample");
    }
  } else if (command == "cal clear") {
    preferences.clear();
    calibration = Calibration{};
    gyroCalibrationActive = false;
    leftInverted = true;
    rightInverted = true;
    currentServoDeg = Threshold::SERVO_CENTER_DEG;
    if (servoReady) writeScanServo(currentServoDeg);
    armed = false;
    activeBrake(false);
    Serial.println("CALIBRATION CLEARED; auto locked");
  } else {
    Serial.println(
        "Commands: status front stop fwd rev left right");
    Serial.println(
        "Calibration: cal motors <L 0|1> <R 0|1> | cal servo <center> <left|right> | cal width/track/tickum/turn90 | cal imulevel/gyrobias | cal waterdry/waterwet/gasbase/clear");
  }
}

void serviceSerial() {
  uint16_t budget = 0;
  while (Serial.available() && budget++ < MAX_SERIAL_LINE) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (serialLine.length()) {
        handleSerialCommand(serialLine);
        serialLine = "";
      }
    } else if (serialLine.length() < MAX_SERIAL_LINE) {
      serialLine += c;
    } else {
      serialLine = "";
      Serial.println("INPUT_TOO_LONG");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(250);
  pinMode(Pin::LEFT_PWM, OUTPUT);
  pinMode(Pin::RIGHT_PWM, OUTPUT);
  pinMode(Pin::LEFT_IN1, OUTPUT);
  pinMode(Pin::LEFT_IN2, OUTPUT);
  pinMode(Pin::RIGHT_IN1, OUTPUT);
  pinMode(Pin::RIGHT_IN2, OUTPUT);
  pinMode(Pin::TRIG, OUTPUT);
  pinMode(Pin::ECHO, INPUT);
  pinMode(Pin::RED, OUTPUT);
  pinMode(Pin::GREEN, OUTPUT);
  pinMode(Pin::BUZZER, OUTPUT);
  pinMode(Pin::LEFT_ENCODER, INPUT);
  pinMode(Pin::RIGHT_ENCODER, INPUT);
  digitalWrite(Pin::TRIG, LOW);
  digitalWrite(Pin::BUZZER, LOW);
  motorPwmReady =
      attachMotorPwm(Pin::LEFT_PWM, LEFT_PWM_CHANNEL_V2) &&
      attachMotorPwm(Pin::RIGHT_PWM, RIGHT_PWM_CHANNEL_V2);
  activeBrake(false);
  attachInterrupt(digitalPinToInterrupt(Pin::LEFT_ENCODER),
                  onLeftEncoder, RISING);
  attachInterrupt(digitalPinToInterrupt(Pin::RIGHT_ENCODER),
                  onRightEncoder, RISING);
  analogReadResolution(12);
  analogSetPinAttenuation(Pin::GAS, ADC_11db);
  analogSetPinAttenuation(Pin::WATER, ADC_11db);
  Wire.begin(Pin::SDA, Pin::SCL);
  Wire.setClock(100000);
  imuReady = mpu.begin(Pin::MPU6050_ADDRESS, &Wire);
  if (imuReady) {
    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
  tofReady = tof.begin(Pin::VL53L0X_ADDRESS, false, &Wire);
  loadCalibration();
  servoReady = attachScanServo();
  currentServoDeg = calibration.servo_center_deg
                        ? calibration.servo_center_deg
                        : Threshold::SERVO_CENTER_DEG;
  if (servoReady) writeScanServo(currentServoDeg);
  dht.begin();
  WiFi.mode(WIFI_STA);
  Serial.printf("ROVER_STA_MAC=%s\n", WiFi.macAddress().c_str());
  radioReady = initializeRadio();
  lastLeftEncoderMotionAtMs = millis();
  lastRightEncoderMotionAtMs = millis();
  sampleFront(true);
  Serial.println(
      "\n=== DEEPTRACK ROVER MISSION / DEFAULT DISARMED ===");
  Serial.printf("PWM=%u IMU=%u TOF=%u SERVO=%u RADIO=%u\n",
                motorPwmReady, imuReady, tofReady, servoReady,
                radioReady);
  if (!radioReady)
    Serial.println(
        "RADIO_DISABLED: provision measured MACs and nonzero PMK/LMK in ignored local config");
  printStatus();
  sendEvent(Protocol::EventCode::BOOT, 0, esp_reset_reason());
}

void loop() {
  drainCommand();
  drainDiagnostic();
  sampleFront(false);
  serviceAnalogSensors();
  serviceImu();
  serviceDht();
  serviceAuto();
  serviceMotorRamp();
  serviceStallDetection();
  serviceBuzzer();
  serviceDiagnosticOutputs();
  serviceSerial();
  sendTelemetry();

  const uint32_t now = millis();
  if (armed && acceptedCommandTtlMs &&
      elapsed(now, lastValidCommandAtMs, acceptedCommandTtlMs)) {
    armed = false;
    autoPhase = AutoPhase::IDLE;
    observationOnlyScan = false;
    activeBrake(true);
    sendEvent(Protocol::EventCode::COMMAND_EXPIRED, 2);
  }
  if (benchMotionUntilMs &&
      static_cast<int32_t>(now - benchMotionUntilMs) >= 0) {
    activeBrake(false);
    Serial.println("BENCH_BURST_EXPIRED: active brake");
  }
}
