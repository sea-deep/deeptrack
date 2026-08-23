#pragma once
#include <stdint.h>

/*
  Shared telemetry packet for ESP-NOW communication between
  the Rover ESP32 and the Gateway ESP32.
  48 bytes — well under ESP-NOW's 250-byte limit.

  Also used by the gateway to parse Serial2 bridge JSON
  and to relay JSON to the laptop dashboard.
*/

typedef struct __attribute__((packed)) {
  float    tempC;        // DHT22 temperature (°C)
  float    humidity;     // DHT22 relative humidity (%)
  int16_t  gasRaw;       // MQ-4 Methane sensor ADC (0-4095)
  float    frontCm;      // HC-SR04 front distance (cm)
  int16_t  tofRaw;       // VL53L0X sim pot ADC (0-4095)
  float    tiltDeg;      // MPU6050 tilt angle (degrees)
  int16_t  waterRaw;     // Water sensor ADC (0-4095)
  uint32_t encL, encR;   // Wheel encoder pulse counts
  float    x, y;         // Dead-reckoned position (cm)
  float    heading;      // Heading (degrees, 0=+X, CCW positive)
  uint8_t  state;        // 0=NORMAL 1=SLOW 2=AVOIDING 3=DANGER
  uint8_t  dangerCause;  // 0=NONE 1=GAS 2=TILT 3=WATER 4=TEMP 5=HUMIDITY 6=TRAPPED
} TelemetryPacket;

// String lookups for JSON serialization / LCD display
static const char* const STATE_NAMES[]  = {"NORMAL","SLOW","AVOIDING","DANGER"};
static const char* const DANGER_NAMES[] = {"NONE","GAS","TILT","WATER","TEMP","HUMIDITY","TRAPPED"};
