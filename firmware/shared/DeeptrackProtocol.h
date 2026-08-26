#pragma once

#include <stddef.h>
#include <stdint.h>

// Shared binary contract for the rover <-> gateway ESP-NOW link.
// Both ESP32 peers use the same little-endian target. The laptop receives
// validated NDJSON from the gateway rather than these packed structs directly.
namespace DeepTrack {
namespace Protocol {

constexpr uint16_t PACKET_MAGIC = 0xD33F;
constexpr uint8_t PROTOCOL_VERSION = 1;
constexpr size_t ESP_NOW_V1_MAX_PAYLOAD_BYTES = 250;

constexpr uint16_t BROWSER_HEARTBEAT_INTERVAL_MS = 150;
constexpr uint16_t GATEWAY_COMMAND_INTERVAL_MS = 125;
constexpr uint16_t DEFAULT_COMMAND_TTL_MS = 300;
constexpr uint16_t MAX_COMMAND_TTL_MS = 500;
constexpr uint16_t TELEMETRY_INTERVAL_MS = 125;

constexpr uint16_t UNKNOWN_DISTANCE_MM = UINT16_MAX;
constexpr uint8_t UNKNOWN_CONFIDENCE_PCT = UINT8_MAX;
constexpr int8_t RSSI_UNAVAILABLE_DBM = INT8_MIN;

enum class MessageType : uint8_t {
  TELEMETRY = 1,
  COMMAND = 2,
  SCAN = 3,
  EVENT = 4,
  APP_ACK = 5,
  DIAGNOSTIC_COMMAND = 6,
  DIAGNOSTIC_RESULT = 7,
};

enum class DiagnosticAction : uint8_t {
  STATUS = 1,
  I2C_SCAN = 2,
  DHT = 3,
  IMU = 4,
  GAS = 5,
  WATER = 6,
  FRONT_RANGE = 7,
  TOF = 8,
  ENCODERS = 9,
  START_SCAN = 10,
  SERVO = 11,
  LEDS = 12,
  BUZZER = 13,
  RADIO_LINK = 14,
  MOTOR_STATE = 15,
  SENSOR_FRESHNESS = 16,
  CALIBRATION_STATUS = 17,
  CALIBRATE_IMU = 18,
};

enum class DiagnosticStatus : uint8_t {
  OK = 0,
  WARNING = 1,
  FAILED = 2,
  REJECTED = 3,
};

enum DiagnosticFlag : uint16_t {
  DIAG_IMU_READY = 1u << 0,
  DIAG_TOF_READY = 1u << 1,
  DIAG_SERVO_READY = 1u << 2,
  DIAG_DHT_READY = 1u << 3,
  DIAG_FRONT_VALID = 1u << 4,
  DIAG_RADIO_READY = 1u << 5,
  DIAG_LEFT_ENCODER_HIGH = 1u << 6,
  DIAG_RIGHT_ENCODER_HIGH = 1u << 7,
  DIAG_CALIBRATION_READY = 1u << 8,
  DIAG_I2C_TOF_FOUND = 1u << 9,
  DIAG_I2C_IMU_FOUND = 1u << 10,
  DIAG_ARMED = 1u << 11,
  DIAG_BRAKE_LATCHED = 1u << 12,
  DIAG_STALL_MONITOR_READY = 1u << 13,
};

enum class DriveCommand : uint8_t {
  STOP = 0,
  FORWARD = 1,
  REVERSE = 2,
  LEFT = 3,
  RIGHT = 4,
  AUTO = 5,
  MANUAL = 6,
  DRIVE = 7,
};

enum class DriveState : uint8_t {
  STOPPED = 0,
  MANUAL = 1,
  AUTO_ADVANCE = 2,
  AUTO_SCAN = 3,
  AUTO_TURN = 4,
  SAFE_STOP = 5,
  STUCK = 6,
};

enum class SensorState : uint8_t {
  UNKNOWN = 0,
  RAW_ONLY = 1,
  WARMING = 2,
  QUALITATIVE_READY = 3,
  CONTACT = 4,
  SENSOR_FAULT = 5,
  QUALITATIVE_ADVISORY = 6,
  DRY_BASELINE = 7,
};

enum StatusFlag : uint16_t {
  FRONT_VALID = 1u << 0,
  FRONT_FRESH = 1u << 1,
  FRONT_BLOCKED = 1u << 2,
  TOF_VALID = 1u << 3,
  IMU_VALID = 1u << 4,
  DHT_VALID = 1u << 5,
  GAS_BASELINED = 1u << 6,
  WATER_CALIBRATED = 1u << 7,
  WATER_CONTACT = 1u << 8,
  LINK_FRESH = 1u << 9,
  RADIO_CONFIGURED = 1u << 10,
  COMMAND_ARMED = 1u << 11,
  AUTO_ENABLED = 1u << 12,
  ODOMETRY_DEGRADED = 1u << 13,
  STALL_SUSPECTED = 1u << 14,
  CALIBRATION_INCOMPLETE = 1u << 15,
};

enum class AckStatus : uint8_t {
  ACCEPTED = 0,
  REJECTED = 1,
  DUPLICATE = 2,
};

enum class AckReason : uint8_t {
  NONE = 0,
  BAD_HEADER = 1,
  BAD_SOURCE = 2,
  BAD_SESSION = 3,
  BAD_SEQUENCE = 4,
  BAD_TTL = 5,
  DISARMED = 6,
  FRONT_BLOCKED = 7,
  SENSOR_UNAVAILABLE = 8,
  CALIBRATION_REQUIRED = 9,
};

enum class EventCode : uint16_t {
  BOOT = 1,
  FRONT_BLOCKED = 2,
  FRONT_STALE = 3,
  COMMAND_EXPIRED = 4,
  COMMAND_REJECTED = 5,
  ACTIVE_BRAKE = 6,
  STALL_SUSPECTED = 7,
  WATER_CONTACT = 8,
  AUTO_STUCK = 9,
  LINK_RESTORED = 10,
  TILT_STOP = 11,
  GAS_ADVISORY = 12,
  CALIBRATION_CHANGED = 13,
  AUTO_STARTED = 14,
  AUTO_TURN_COMPLETE = 15,
};

struct __attribute__((packed)) PacketHeader {
  uint16_t magic;
  uint8_t version;
  uint8_t type;
  uint32_t session_id;
  uint32_t sequence;
  uint32_t sender_uptime_ms;  // Diagnostic only; peer clocks are not synchronized.
};

struct __attribute__((packed)) TelemetryPacket {
  PacketHeader header;
  int32_t left_ticks;
  int32_t right_ticks;
  uint16_t heading_cdeg;
  int16_t pitch_cdeg;
  int16_t roll_cdeg;
  uint16_t front_mm;
  uint16_t tof_mm;
  uint16_t gas_raw;
  uint16_t gas_pin_mv;
  uint16_t water_raw;
  uint16_t water_pin_mv;
  uint16_t chassis_width_mm;
  uint16_t track_width_mm;
  uint16_t micrometers_per_tick;
  int16_t temperature_centi_c;
  uint16_t humidity_centi_pct;
  uint16_t status_flags;
  uint8_t servo_deg;
  uint8_t drive_state;
  int8_t last_rssi_dbm;
  uint8_t reset_reason;
  uint8_t gas_state;
  uint8_t water_state;
};

struct __attribute__((packed)) CommandPacket {
  PacketHeader header;
  uint8_t command;
  int8_t left_percent;
  int8_t right_percent;
  uint16_t ttl_ms;
  uint8_t mode;
};

struct __attribute__((packed)) ScanPacket {
  PacketHeader header;
  uint32_t scan_id;
  uint16_t angle_cdeg;
  uint16_t distance_mm;
  uint8_t valid;
  uint8_t range_status;
  uint8_t confidence_pct;
  uint8_t reserved;
};

struct __attribute__((packed)) AppAckPacket {
  PacketHeader header;
  uint32_t acknowledged_sequence;
  uint8_t status;
  uint8_t reason;
  uint16_t reserved;
};

struct __attribute__((packed)) EventPacket {
  PacketHeader header;
  uint16_t code;
  uint8_t severity;
  uint8_t reserved;
  int32_t value;
};

struct __attribute__((packed)) DiagnosticCommandPacket {
  PacketHeader header;
  uint8_t action;
  int16_t argument;
  uint8_t reserved;
};

struct __attribute__((packed)) DiagnosticResultPacket {
  PacketHeader header;
  uint32_t request_sequence;
  uint8_t action;
  uint8_t status;
  uint16_t flags;
  int32_t value_a;
  int32_t value_b;
  int32_t value_c;
  int32_t value_d;
};

static_assert(sizeof(PacketHeader) == 16,
              "PacketHeader wire layout changed; bump the protocol version intentionally");
static_assert(sizeof(TelemetryPacket) == 60,
              "TelemetryPacket wire layout changed; bump the protocol version intentionally");
static_assert(sizeof(CommandPacket) == 22,
              "CommandPacket wire layout changed; bump the protocol version intentionally");
static_assert(sizeof(ScanPacket) == 28,
              "ScanPacket wire layout changed; bump the protocol version intentionally");
static_assert(sizeof(AppAckPacket) == 24,
              "AppAckPacket wire layout changed; bump the protocol version intentionally");
static_assert(sizeof(EventPacket) == 24,
              "EventPacket wire layout changed; bump the protocol version intentionally");
static_assert(sizeof(DiagnosticCommandPacket) == 20,
              "Diagnostic command wire layout changed; bump the protocol version intentionally");
static_assert(sizeof(DiagnosticResultPacket) == 40,
              "Diagnostic result wire layout changed; bump the protocol version intentionally");
static_assert(sizeof(TelemetryPacket) <= ESP_NOW_V1_MAX_PAYLOAD_BYTES,
              "Telemetry packet exceeds the ESP-NOW v1 payload limit");
static_assert(sizeof(CommandPacket) <= ESP_NOW_V1_MAX_PAYLOAD_BYTES,
              "Command packet exceeds the ESP-NOW v1 payload limit");
static_assert(sizeof(ScanPacket) <= ESP_NOW_V1_MAX_PAYLOAD_BYTES,
              "Scan packet exceeds the ESP-NOW v1 payload limit");
static_assert(DEFAULT_COMMAND_TTL_MS <= MAX_COMMAND_TTL_MS,
              "Default command TTL exceeds the hard safety maximum");

}  // namespace Protocol
}  // namespace DeepTrack
