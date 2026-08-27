#pragma once

#include <stdint.h>

// Central authority for software starting values. These are conservative
// commissioning defaults from the implementation playbook, not measurements of
// the assembled rover. Values that depend on geometry or a physical sensor are
// persisted only after an explicit calibration command.
namespace DeepTrack {
namespace Thresholds {

constexpr float FRONT_STOP_CM = 24.0f;
constexpr float FRONT_CLEAR_CM = 32.0f;
constexpr float FRONT_MIN_CM = 2.0f;
constexpr float FRONT_MAX_CM = 200.0f;
constexpr uint32_t FRONT_SAMPLE_INTERVAL_MS = 80;
constexpr uint32_t FRONT_STALE_AFTER_MS = 300;
constexpr uint32_t CENTER_TOF_SAMPLE_INTERVAL_MS = 125;
constexpr uint32_t FRONT_FUSION_STALE_MS = 300;
constexpr uint32_t ULTRASONIC_TIMEOUT_US = 12000;

constexpr int16_t MANUAL_DUTY = 145;
constexpr int16_t AUTO_DUTY = 125;
constexpr int16_t TURN_DUTY = 120;
constexpr uint32_t MOTOR_RAMP_INTERVAL_MS = 30;
constexpr uint8_t MOTOR_RAMP_STEP = 12;
constexpr uint32_t SERIAL_BENCH_BURST_MS = 600;
constexpr uint32_t VALIDATION_COUNTDOWN_MS = 5000;
constexpr uint32_t VALIDATION_MAX_LIFTED_RUN_MS = 3000;
constexpr uint32_t VALIDATION_MAX_GROUND_RUN_MS = 5000;
constexpr uint32_t VALIDATION_MIN_NOISE_MS = 10000;
constexpr uint32_t VALIDATION_MAX_NOISE_MS = 60000;
constexpr uint32_t VALIDATION_LEASE_TIMEOUT_MS = 750;
constexpr int16_t VALIDATION_MIN_DUTY = 80;
constexpr int16_t VALIDATION_MAX_DUTY = 170;
constexpr uint32_t STALL_WINDOW_MS = 650;
constexpr int16_t STALL_MIN_DUTY = 100;

// LM393 starting values only. GPIO34/35 have no internal pull-ups; the
// external pull-ups and FALLING-edge electrical behavior still require a
// wheels-up signal test before these values can be called calibrated.
constexpr uint32_t ENCODER_MIN_PULSE_US = 1500;
constexpr int16_t ENCODER_MOTOR_ACTIVE_DUTY = 25;
constexpr uint32_t ENCODER_COAST_GRACE_MS = 80;
constexpr bool ENCODER_USE_MOTOR_STATE_GATE = true;

constexpr uint8_t SCAN_ANGLES_DEG[] = {35, 55, 75, 90, 105, 125, 145};
constexpr uint8_t SCAN_ANGLE_COUNT =
    sizeof(SCAN_ANGLES_DEG) / sizeof(SCAN_ANGLES_DEG[0]);
constexpr uint8_t SERVO_CENTER_DEG = 90;
constexpr uint32_t SERVO_SETTLE_MS = 110;
constexpr uint32_t MANUAL_SCAN_COOLDOWN_MS = 2000;
constexpr uint16_t TOF_MIN_MM = 30;
constexpr uint16_t TOF_MAX_MM = 1200;
constexpr uint16_t OPENING_MARGIN_EACH_SIDE_MM = 50;
constexpr uint16_t OPTICAL_UNCERTAINTY_MM = 80;

constexpr uint32_t GAS_INTERVAL_MS = 100;
constexpr uint8_t GAS_MEDIAN_WINDOW = 5;
constexpr float GAS_EMA_ALPHA = 0.20f;
constexpr uint32_t GAS_BOOT_STABILIZE_MS = 60000;
constexpr uint8_t GAS_ADVISORY_PERSIST_SAMPLES = 10;
constexpr uint16_t GAS_MIN_ADVISORY_DELTA = 120;

constexpr uint32_t WATER_INTERVAL_MS = 100;
constexpr uint8_t WATER_CONTACT_PERSIST_SAMPLES = 3;
constexpr uint8_t WATER_CLEAR_PERSIST_SAMPLES = 5;
constexpr uint16_t WATER_MIN_CALIBRATION_SPAN = 100;
constexpr uint8_t WATER_CONTACT_PERCENT_OF_SPAN = 60;
constexpr uint8_t WATER_CLEAR_PERCENT_OF_SPAN = 40;

constexpr uint32_t IMU_INTERVAL_MS = 50;
constexpr uint32_t GYRO_BIAS_CALIBRATION_MS = 3000;
constexpr uint16_t GYRO_BIAS_MIN_SAMPLES = 40;
constexpr float GYRO_BIAS_MAX_STDDEV_DPS = 0.50f;
constexpr float GYRO_BIAS_MAX_ABS_DPS = 5.0f;
constexpr float TILT_CAUTION_DEG = 12.0f;
constexpr float TILT_STOP_DEG = 25.0f;
constexpr float TILT_CLEAR_DEG = 20.0f;
constexpr uint32_t TILT_STOP_PERSIST_MS = 500;
constexpr uint32_t DHT_INTERVAL_MS = 2500;

static_assert(FRONT_CLEAR_CM > FRONT_STOP_CM,
              "Front hysteresis clear point must exceed stop point");
static_assert(FRONT_STALE_AFTER_MS <= 500,
              "Front sensing must fail closed inside the command TTL ceiling");
static_assert(SCAN_ANGLE_COUNT >= 3,
              "Opening validation requires adjacent scan sectors");
static_assert(TILT_CLEAR_DEG < TILT_STOP_DEG,
              "Tilt clear threshold must be below stop threshold");
static_assert(WATER_CLEAR_PERCENT_OF_SPAN < WATER_CONTACT_PERCENT_OF_SPAN,
              "Water contact hysteresis thresholds are reversed");
static_assert(ENCODER_MOTOR_ACTIVE_DUTY >= 0 &&
                  ENCODER_MOTOR_ACTIVE_DUTY <= 255,
              "Encoder motor-state threshold must be a valid PWM duty");
static_assert(VALIDATION_MAX_DUTY <= 255 &&
                  VALIDATION_MIN_DUTY <= VALIDATION_MAX_DUTY,
              "Validation PWM bounds are invalid");

}  // namespace Thresholds
}  // namespace DeepTrack
