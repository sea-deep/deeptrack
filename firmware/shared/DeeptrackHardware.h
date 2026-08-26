#pragma once

#include <stdint.h>

// Firmware pin authority for the physical DEEPTRACK build.
// Electrical details, voltage dividers, and power rails remain authoritative in
// docs/WIRING_GUIDE.md. Do not add a TB6612 STBY GPIO: both STBY inputs are
// physically tied to 3V3.
namespace DeepTrack {
namespace Hardware {

namespace Rover {
constexpr uint8_t DHT_DATA = 23;
constexpr uint8_t GAS = 36;
constexpr uint8_t WATER = 39;

constexpr uint8_t TRIG = 19;
constexpr uint8_t ECHO = 18;

constexpr uint8_t SDA = 21;
constexpr uint8_t SCL = 22;
constexpr uint8_t MPU6050_ADDRESS = 0x68;
constexpr uint8_t VL53L0X_ADDRESS = 0x29;

constexpr uint8_t LEFT_ENCODER = 34;
constexpr uint8_t RIGHT_ENCODER = 35;

constexpr uint8_t SERVO = 13;
constexpr uint8_t BUZZER = 4;
constexpr uint8_t RED = 26;
constexpr uint8_t GREEN = 27;

// Each driver fans one control triplet out to its front and rear channels.
constexpr uint8_t LEFT_PWM = 25;
constexpr uint8_t LEFT_IN1 = 14;
constexpr uint8_t LEFT_IN2 = 16;
constexpr uint8_t RIGHT_PWM = 17;
constexpr uint8_t RIGHT_IN1 = 33;
constexpr uint8_t RIGHT_IN2 = 32;

constexpr bool MOTOR_STBY_HARDWIRED_HIGH = true;
}  // namespace Rover

namespace Gateway {
constexpr uint8_t SDA = 21;
constexpr uint8_t SCL = 22;
constexpr uint8_t LCD_ADDRESS = 0x27;
constexpr uint8_t LED_RED = 25;
constexpr uint8_t LED_YELLOW = 26;
constexpr uint8_t LED_GREEN = 27;
}  // namespace Gateway

static_assert(Rover::GAS >= 32 && Rover::GAS <= 39,
              "MQ-4 must remain on an ESP32 ADC1-capable input");
static_assert(Rover::WATER >= 32 && Rover::WATER <= 39,
              "Water sensor must remain on an ESP32 ADC1-capable input");
static_assert(Rover::LEFT_ENCODER >= 34 && Rover::RIGHT_ENCODER >= 34,
              "Encoder inputs are expected on GPIO34-39 with external pull-ups");

}  // namespace Hardware
}  // namespace DeepTrack
