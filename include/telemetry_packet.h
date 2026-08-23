#pragma once

#include <stdint.h>

// Standard telemetry (sent slowly)
typedef struct __attribute__((packed)) {
    float temperature;
    float humidity;
    float ax, ay, az;
    float gx, gy, gz;
    uint16_t gasRaw;
    uint16_t waterRaw;
    uint32_t dangerState; // 0=safe, 1=danger
} TelemetryPacket;

// Fast scan telemetry (sent rapidly)
typedef struct __attribute__((packed)) {
    uint8_t type;         // 1 = scan
    uint8_t seq;          // Sequence number
    int16_t angle_deg;    // Servo angle in degrees
    uint16_t distance_mm; // VL53L0X distance
    uint8_t valid;        // 1 if valid, 0 if out of range
    uint32_t timestamp_ms; // timestamp
} ScanPacket;

// Gateway to Rover control packet (Heartbeat)
typedef struct __attribute__((packed)) {
    uint8_t type;         // 0 = command
    int16_t motor_l;      // -255 to 255
    int16_t motor_r;      // -255 to 255
} ControlPacket;

