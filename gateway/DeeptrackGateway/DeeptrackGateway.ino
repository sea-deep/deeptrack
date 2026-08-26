// DEEPTRACK production gateway: encrypted ESP-NOW <-> USB NDJSON bridge.

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_now.h>
#include <esp_system.h>
#include <esp_wifi.h>

#include "../../firmware/shared/DeeptrackHardware.h"
#include "../../firmware/shared/DeeptrackProtocol.h"
#include "../../firmware/shared/DeeptrackRadioConfig.h"

namespace Pin = DeepTrack::Hardware::Gateway;
namespace Protocol = DeepTrack::Protocol;
namespace Radio = DeepTrack::RadioConfig;

constexpr uint32_t OPERATOR_HEARTBEAT_TIMEOUT_MS = 450;
constexpr uint32_t ROVER_LINK_STALE_MS = 500;
constexpr uint32_t HELLO_INTERVAL_MS = 2000;
constexpr uint32_t LCD_INTERVAL_MS = 500;
constexpr size_t MAX_SERIAL_LINE = 384;

LiquidCrystal_I2C lcd(Pin::LCD_ADDRESS, 16, 2);
portMUX_TYPE radioMux = portMUX_INITIALIZER_UNLOCKED;

volatile bool telemetryPending = false;
volatile bool scanPending = false;
volatile bool eventPending = false;
volatile bool ackPending = false;
volatile bool diagnosticResultPending = false;
volatile int8_t pendingRssiDbm = Protocol::RSSI_UNAVAILABLE_DBM;
Protocol::TelemetryPacket pendingTelemetry{};
Protocol::ScanPacket pendingScan{};
Protocol::EventPacket pendingEvent{};
Protocol::AppAckPacket pendingAck{};
Protocol::DiagnosticResultPacket pendingDiagnosticResult{};

bool radioReady = false;
bool lcdReady = false;
bool operatorArmed = false;
bool operatorWasFresh = false;
bool roverWasFresh = false;
uint32_t gatewaySession = 0;
uint32_t commandSequence = 0;
uint32_t lastBrowserSequence = 0;
uint32_t lastHeartbeatAtMs = 0;
uint32_t lastRoverPacketAtMs = 0;
uint32_t lastHelloAtMs = 0;
uint32_t lastCommandSendAtMs = 0;
uint32_t lastLcdAtMs = 0;
uint32_t lastRoverSequence = 0;
uint32_t lastRoverUptimeMs = 0;
uint32_t radioGapCount = 0;
uint32_t pendingOverwriteCount = 0;
uint32_t sendFailureCount = 0;
int8_t lastRssiDbm = Protocol::RSSI_UNAVAILABLE_DBM;
int8_t requestedLeftPct = 0;
int8_t requestedRightPct = 0;
Protocol::DriveCommand requestedCommand = Protocol::DriveCommand::STOP;
String serialLine;
String lcdLine1;
String lcdLine2;

bool macEquals(const uint8_t* first, const uint8_t* second) {
  return first && second && memcmp(first, second, 6) == 0;
}

bool nonzeroBytes(const uint8_t* data, size_t length) {
  uint8_t combined = 0;
  for (size_t i = 0; i < length; ++i) combined |= data[i];
  return combined != 0;
}

bool radioConfigurationValid() {
  return Radio::ENABLED && Radio::CHANNEL >= 1 && Radio::CHANNEL <= 14 &&
         nonzeroBytes(Radio::ROVER_MAC, 6) &&
         nonzeroBytes(Radio::GATEWAY_MAC, 6) && nonzeroBytes(Radio::PMK, 16) &&
         nonzeroBytes(Radio::LMK, 16);
}

bool packetHeaderValid(const Protocol::PacketHeader& header,
                       Protocol::MessageType expectedType) {
  return header.magic == Protocol::PACKET_MAGIC &&
         header.version == Protocol::PROTOCOL_VERSION &&
         header.type == static_cast<uint8_t>(expectedType);
}

void onRadioSend(const esp_now_send_info_t*, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) ++sendFailureCount;
}

void onRadioReceive(const esp_now_recv_info_t* info, const uint8_t* data,
                    int length) {
  if (!info || !data || !macEquals(info->src_addr, Radio::ROVER_MAC) ||
      length < static_cast<int>(sizeof(Protocol::PacketHeader))) {
    return;
  }

  Protocol::PacketHeader header{};
  memcpy(&header, data, sizeof(header));
  if (header.magic != Protocol::PACKET_MAGIC ||
      header.version != Protocol::PROTOCOL_VERSION) {
    return;
  }

  int8_t rssi = Protocol::RSSI_UNAVAILABLE_DBM;
  if (info->rx_ctrl) rssi = static_cast<int8_t>(info->rx_ctrl->rssi);

  portENTER_CRITICAL(&radioMux);
  bool acceptedPacket = false;
  if (header.type == static_cast<uint8_t>(Protocol::MessageType::TELEMETRY) &&
      length == static_cast<int>(sizeof(Protocol::TelemetryPacket))) {
    if (telemetryPending) ++pendingOverwriteCount;
    memcpy(&pendingTelemetry, data, sizeof(pendingTelemetry));
    telemetryPending = true;
    acceptedPacket = true;
  } else if (header.type == static_cast<uint8_t>(Protocol::MessageType::SCAN) &&
             length == static_cast<int>(sizeof(Protocol::ScanPacket))) {
    if (scanPending) ++pendingOverwriteCount;
    memcpy(&pendingScan, data, sizeof(pendingScan));
    scanPending = true;
    acceptedPacket = true;
  } else if (header.type == static_cast<uint8_t>(Protocol::MessageType::EVENT) &&
             length == static_cast<int>(sizeof(Protocol::EventPacket))) {
    if (eventPending) ++pendingOverwriteCount;
    memcpy(&pendingEvent, data, sizeof(pendingEvent));
    eventPending = true;
    acceptedPacket = true;
  } else if (header.type == static_cast<uint8_t>(Protocol::MessageType::APP_ACK) &&
             length == static_cast<int>(sizeof(Protocol::AppAckPacket))) {
    if (ackPending) ++pendingOverwriteCount;
    memcpy(&pendingAck, data, sizeof(pendingAck));
    ackPending = true;
    acceptedPacket = true;
  } else if (header.type == static_cast<uint8_t>(
                 Protocol::MessageType::DIAGNOSTIC_RESULT) &&
             length == static_cast<int>(
                 sizeof(Protocol::DiagnosticResultPacket))) {
    if (diagnosticResultPending) ++pendingOverwriteCount;
    memcpy(&pendingDiagnosticResult, data, sizeof(pendingDiagnosticResult));
    diagnosticResultPending = true;
    acceptedPacket = true;
  }
  if (acceptedPacket) {
    // Sequence numbers are global across telemetry, scans, events, and ACKs.
    // A rover reboot resets both sequence and uptime; do not count that as an
    // enormous RF loss burst.
    if (lastRoverSequence && lastRoverUptimeMs > header.sender_uptime_ms &&
        lastRoverUptimeMs - header.sender_uptime_ms > 10000u) {
      lastRoverSequence = 0;
      radioGapCount = 0;
    }
    if (lastRoverSequence) {
      const int32_t advance =
          static_cast<int32_t>(header.sequence - lastRoverSequence);
      if (advance > 1) radioGapCount += static_cast<uint32_t>(advance - 1);
    }
    if (!lastRoverSequence ||
        static_cast<int32_t>(header.sequence - lastRoverSequence) > 0) {
      lastRoverSequence = header.sequence;
      lastRoverUptimeMs = header.sender_uptime_ms;
    }
    pendingRssiDbm = rssi;
  }
  portEXIT_CRITICAL(&radioMux);
}

void writeJson(JsonDocument& document) {
  serializeJson(document, Serial);
  Serial.println();
}

void emitHello() {
  JsonDocument document;
  document["type"] = "hello";
  document["protocol"] = Protocol::PROTOCOL_VERSION;
  document["board"] = "gateway";
  document["gateway_sta_mac"] = WiFi.macAddress();
  document["session"] = gatewaySession;
  document["radio_ready"] = radioReady;
  document["lcd_ready"] = lcdReady;
  document["armed"] = operatorArmed;
  writeJson(document);
}

void emitGatewayEvent(const char* severity, const char* code,
                      const char* message) {
  JsonDocument document;
  document["type"] = "event";
  document["source"] = "gateway";
  document["severity"] = severity;
  document["code"] = code;
  document["message"] = message;
  document["timestamp_ms"] = millis();
  writeJson(document);
}

const char* sensorStateName(uint8_t state) {
  switch (static_cast<Protocol::SensorState>(state)) {
    case Protocol::SensorState::UNKNOWN: return "UNKNOWN";
    case Protocol::SensorState::RAW_ONLY: return "RAW_ONLY";
    case Protocol::SensorState::WARMING: return "WARMING";
    case Protocol::SensorState::QUALITATIVE_READY: return "QUALITATIVE_READY";
    case Protocol::SensorState::CONTACT: return "CONTACT";
    case Protocol::SensorState::SENSOR_FAULT: return "SENSOR_FAULT";
    case Protocol::SensorState::QUALITATIVE_ADVISORY: return "QUALITATIVE_ADVISORY";
    case Protocol::SensorState::DRY_BASELINE: return "DRY_BASELINE";
  }
  return "UNKNOWN";
}

const char* driveStateName(uint8_t state) {
  switch (static_cast<Protocol::DriveState>(state)) {
    case Protocol::DriveState::STOPPED: return "STOPPED";
    case Protocol::DriveState::MANUAL: return "MANUAL";
    case Protocol::DriveState::AUTO_ADVANCE: return "AUTO_ADVANCE";
    case Protocol::DriveState::AUTO_SCAN: return "AUTO_SCAN";
    case Protocol::DriveState::AUTO_TURN: return "AUTO_TURN";
    case Protocol::DriveState::SAFE_STOP: return "SAFE_STOP";
    case Protocol::DriveState::STUCK: return "STUCK";
  }
  return "STOPPED";
}

void emitTelemetry(const Protocol::TelemetryPacket& packet, int8_t rssi) {
  const uint16_t flags = packet.status_flags;
  JsonDocument document;
  document["type"] = "telemetry";
  document["source"] = "LIVE";
  document["session"] = packet.header.session_id;
  document["seq"] = packet.header.sequence;
  document["timestamp_ms"] = packet.header.sender_uptime_ms;
  document["gateway_received_ms"] = millis();
  document["front_valid"] = (flags & Protocol::FRONT_VALID) != 0;
  document["front_fresh"] = (flags & Protocol::FRONT_FRESH) != 0;
  document["front_blocked"] = (flags & Protocol::FRONT_BLOCKED) != 0;
  if (packet.front_mm == Protocol::UNKNOWN_DISTANCE_MM) document["front_cm"] = nullptr;
  else document["front_cm"] = packet.front_mm / 10.0f;
  if (packet.tof_mm == Protocol::UNKNOWN_DISTANCE_MM) document["tof_mm"] = nullptr;
  else document["tof_mm"] = packet.tof_mm;
  document["gas_raw"] = packet.gas_raw;
  document["gas_pin_mv"] = packet.gas_pin_mv;
  document["gas_state"] = sensorStateName(packet.gas_state);
  document["water_raw"] = packet.water_raw;
  document["water_pin_mv"] = packet.water_pin_mv;
  document["water_state"] = sensorStateName(packet.water_state);
  if (packet.chassis_width_mm) document["chassis_width_mm"] = packet.chassis_width_mm;
  else document["chassis_width_mm"] = nullptr;
  if (packet.track_width_mm) document["track_width_mm"] = packet.track_width_mm;
  else document["track_width_mm"] = nullptr;
  if (packet.micrometers_per_tick) document["micrometers_per_tick"] = packet.micrometers_per_tick;
  else document["micrometers_per_tick"] = nullptr;
  if (flags & Protocol::DHT_VALID) {
    document["temp_c"] = packet.temperature_centi_c / 100.0f;
    document["humidity_pct"] = packet.humidity_centi_pct / 100.0f;
  } else {
    document["temp_c"] = nullptr;
    document["humidity_pct"] = nullptr;
  }
  if (flags & Protocol::IMU_VALID) {
    document["pitch_deg"] = packet.pitch_cdeg / 100.0f;
    document["roll_deg"] = packet.roll_cdeg / 100.0f;
    document["heading_deg"] = packet.heading_cdeg / 100.0f;
  } else {
    document["pitch_deg"] = nullptr;
    document["roll_deg"] = nullptr;
    document["heading_deg"] = nullptr;
  }
  document["left_ticks"] = packet.left_ticks;
  document["right_ticks"] = packet.right_ticks;
  document["servo_deg"] = packet.servo_deg;
  document["drive_state"] = driveStateName(packet.drive_state);
  document["status_flags"] = flags;
  if (rssi == Protocol::RSSI_UNAVAILABLE_DBM) {
    document["rssi_dbm"] = nullptr;
  } else {
    document["rssi_dbm"] = rssi;
  }
  document["packet_gaps"] = radioGapCount + pendingOverwriteCount;
  document["reset_reason"] = packet.reset_reason;
  writeJson(document);
}

void emitScan(const Protocol::ScanPacket& packet) {
  JsonDocument document;
  document["type"] = "scan";
  document["source"] = "LIVE";
  document["session"] = packet.header.session_id;
  document["seq"] = packet.header.sequence;
  document["scan_id"] = packet.scan_id;
  document["angle_deg"] = packet.angle_cdeg / 100.0f;
  document["valid"] = packet.valid != 0;
  if (packet.valid) {
    document["distance_mm"] = packet.distance_mm;
  } else {
    document["distance_mm"] = nullptr;
  }
  document["range_status"] = packet.range_status;
  if (packet.confidence_pct == Protocol::UNKNOWN_CONFIDENCE_PCT)
    document["confidence_pct"] = nullptr;
  else
    document["confidence_pct"] = packet.confidence_pct;
  writeJson(document);
}

void emitRoverEvent(const Protocol::EventPacket& packet) {
  JsonDocument document;
  document["type"] = "event";
  document["source"] = "rover";
  document["session"] = packet.header.session_id;
  document["seq"] = packet.header.sequence;
  document["severity"] = packet.severity;
  document["code"] = packet.code;
  document["value"] = packet.value;
  document["timestamp_ms"] = packet.header.sender_uptime_ms;
  writeJson(document);
}

void emitAck(const Protocol::AppAckPacket& packet) {
  JsonDocument document;
  document["type"] = "ack";
  document["session"] = packet.header.session_id;
  document["seq"] = packet.header.sequence;
  document["command_seq"] = packet.acknowledged_sequence;
  document["status"] = packet.status;
  document["reason"] = packet.reason;
  writeJson(document);
}

void emitDiagnosticResult(const Protocol::DiagnosticResultPacket& packet) {
  JsonDocument document;
  document["type"] = "diagnostic";
  document["source"] = "rover";
  document["session"] = packet.header.session_id;
  document["seq"] = packet.header.sequence;
  document["request_seq"] = packet.request_sequence;
  document["action"] = packet.action;
  document["status"] = packet.status;
  document["flags"] = packet.flags;
  document["value_a"] = packet.value_a;
  document["value_b"] = packet.value_b;
  document["value_c"] = packet.value_c;
  document["value_d"] = packet.value_d;
  document["timestamp_ms"] = packet.header.sender_uptime_ms;
  writeJson(document);
}

void drainRadioPackets() {
  Protocol::TelemetryPacket telemetry{};
  Protocol::ScanPacket scan{};
  Protocol::EventPacket event{};
  Protocol::AppAckPacket ack{};
  Protocol::DiagnosticResultPacket diagnostic{};
  bool hasTelemetry = false, hasScan = false, hasEvent = false, hasAck = false,
       hasDiagnostic = false;
  int8_t rssi = Protocol::RSSI_UNAVAILABLE_DBM;

  portENTER_CRITICAL(&radioMux);
  if (telemetryPending) {
    telemetry = pendingTelemetry;
    telemetryPending = false;
    hasTelemetry = true;
  }
  if (scanPending) {
    scan = pendingScan;
    scanPending = false;
    hasScan = true;
  }
  if (eventPending) {
    event = pendingEvent;
    eventPending = false;
    hasEvent = true;
  }
  if (ackPending) {
    ack = pendingAck;
    ackPending = false;
    hasAck = true;
  }
  if (diagnosticResultPending) {
    diagnostic = pendingDiagnosticResult;
    diagnosticResultPending = false;
    hasDiagnostic = true;
  }
  rssi = pendingRssiDbm;
  portEXIT_CRITICAL(&radioMux);

  if (!(hasTelemetry || hasScan || hasEvent || hasAck || hasDiagnostic)) return;
  lastRoverPacketAtMs = millis();
  lastRssiDbm = rssi;
  bool roverSafetyStopped = false;
  if (hasTelemetry && packetHeaderValid(telemetry.header, Protocol::MessageType::TELEMETRY)) {
    emitTelemetry(telemetry, rssi);
    const auto roverDriveState =
        static_cast<Protocol::DriveState>(telemetry.drive_state);
    roverSafetyStopped = operatorArmed &&
        (roverDriveState == Protocol::DriveState::SAFE_STOP ||
         roverDriveState == Protocol::DriveState::STUCK);
  }
  if (hasScan && packetHeaderValid(scan.header, Protocol::MessageType::SCAN)) emitScan(scan);
  if (hasEvent && packetHeaderValid(event.header, Protocol::MessageType::EVENT)) emitRoverEvent(event);
  if (hasAck && packetHeaderValid(ack.header, Protocol::MessageType::APP_ACK)) emitAck(ack);
  if (hasDiagnostic && packetHeaderValid(
          diagnostic.header, Protocol::MessageType::DIAGNOSTIC_RESULT))
    emitDiagnosticResult(diagnostic);
  if (roverSafetyStopped)
    forceStop("Rover entered a safety stop; explicit re-arm required", true);
}

bool sendCommand(Protocol::DriveCommand command, int8_t leftPct,
                 int8_t rightPct, uint16_t ttlMs) {
  if (!radioReady) return false;
  Protocol::CommandPacket packet{};
  packet.header.magic = Protocol::PACKET_MAGIC;
  packet.header.version = Protocol::PROTOCOL_VERSION;
  packet.header.type = static_cast<uint8_t>(Protocol::MessageType::COMMAND);
  packet.header.session_id = gatewaySession;
  packet.header.sequence = ++commandSequence;
  packet.header.sender_uptime_ms = millis();
  packet.command = static_cast<uint8_t>(command);
  packet.left_percent = constrain(leftPct, -100, 100);
  packet.right_percent = constrain(rightPct, -100, 100);
  packet.ttl_ms = min<uint16_t>(ttlMs, Protocol::MAX_COMMAND_TTL_MS);
  packet.mode = operatorArmed ? 1 : 0;
  return esp_now_send(Radio::ROVER_MAC,
                      reinterpret_cast<const uint8_t*>(&packet),
                      sizeof(packet)) == ESP_OK;
}

bool sendDiagnostic(uint8_t action, int16_t argument) {
  if (!radioReady) return false;
  Protocol::DiagnosticCommandPacket packet{};
  packet.header.magic = Protocol::PACKET_MAGIC;
  packet.header.version = Protocol::PROTOCOL_VERSION;
  packet.header.type = static_cast<uint8_t>(
      Protocol::MessageType::DIAGNOSTIC_COMMAND);
  packet.header.session_id = gatewaySession;
  packet.header.sequence = ++commandSequence;
  packet.header.sender_uptime_ms = millis();
  packet.action = action;
  packet.argument = argument;
  return esp_now_send(Radio::ROVER_MAC,
                      reinterpret_cast<const uint8_t*>(&packet),
                      sizeof(packet)) == ESP_OK;
}

void forceStop(const char* reason, bool emit) {
  operatorArmed = false;
  requestedCommand = Protocol::DriveCommand::STOP;
  requestedLeftPct = requestedRightPct = 0;
  sendCommand(Protocol::DriveCommand::STOP, 0, 0,
              Protocol::DEFAULT_COMMAND_TTL_MS);
  if (emit) emitGatewayEvent("warning", "GATEWAY_STOP", reason);
  emitHello();
}

void stopMotion() {
  requestedLeftPct = requestedRightPct = 0;
  sendCommand(Protocol::DriveCommand::STOP, 0, 0,
              Protocol::DEFAULT_COMMAND_TTL_MS);
  // Keep a fresh, explicitly armed manual session alive after the immediate
  // brake. Pointer/key release must not silently resume the previous vector.
  requestedCommand = operatorArmed ? Protocol::DriveCommand::MANUAL
                                   : Protocol::DriveCommand::STOP;
}

bool sessionAndSequenceValid(JsonDocument& document) {
  if (!document["session"].is<uint32_t>() ||
      document["session"].as<uint32_t>() != gatewaySession ||
      !document["seq"].is<uint32_t>()) {
    return false;
  }
  const uint32_t sequence = document["seq"].as<uint32_t>();
  if (lastBrowserSequence && static_cast<int32_t>(sequence - lastBrowserSequence) <= 0) return false;
  lastBrowserSequence = sequence;
  return true;
}

void handleBrowserLine(const String& line) {
  JsonDocument document;
  const DeserializationError error = deserializeJson(document, line);
  if (error || !document["type"].is<const char*>()) {
    emitGatewayEvent("error", "BAD_NDJSON", "Malformed command line rejected");
    return;
  }

  const char* type = document["type"];
  if (strcmp(type, "stop") == 0) {
    stopMotion();
    return;
  }
  if (!sessionAndSequenceValid(document)) {
    emitGatewayEvent("error", "BAD_SESSION_OR_SEQUENCE", "Command rejected");
    return;
  }

  if (strcmp(type, "heartbeat") == 0) {
    lastHeartbeatAtMs = millis();
    return;
  }

  if (strcmp(type, "diagnostic") == 0) {
    if (!document["action"].is<uint8_t>()) {
      emitGatewayEvent("error", "BAD_DIAGNOSTIC",
                       "Diagnostic action is missing or invalid");
      return;
    }
    const uint8_t action = document["action"].as<uint8_t>();
    const int argument = document["argument"] | 0;
    if (action < static_cast<uint8_t>(Protocol::DiagnosticAction::STATUS) ||
        action > static_cast<uint8_t>(
                     Protocol::DiagnosticAction::CALIBRATE_IMU) ||
        argument < -32768 || argument > 32767) {
      emitGatewayEvent("error", "BAD_DIAGNOSTIC",
                       "Diagnostic action or argument is out of range");
      return;
    }
    if (!sendDiagnostic(action, static_cast<int16_t>(argument)))
      emitGatewayEvent("error", "DIAGNOSTIC_SEND_FAILED",
                       "Rover diagnostic request could not be sent");
    return;
  }

  const bool heartbeatFresh = lastHeartbeatAtMs &&
      millis() - lastHeartbeatAtMs <= OPERATOR_HEARTBEAT_TIMEOUT_MS;
  if (strcmp(type, "arm") == 0) {
    const bool roverFresh = lastRoverPacketAtMs &&
        millis() - lastRoverPacketAtMs <= ROVER_LINK_STALE_MS;
    if (!radioReady || !heartbeatFresh || !roverFresh) {
      emitGatewayEvent("error", "ARM_REJECTED", "Radio, rover link, or heartbeat unavailable");
      return;
    }
    operatorArmed = true;
    requestedCommand = Protocol::DriveCommand::MANUAL;
    sendCommand(requestedCommand, 0, 0, Protocol::DEFAULT_COMMAND_TTL_MS);
    emitGatewayEvent("info", "ARMED", "Operator session armed in manual mode");
    emitHello();
    return;
  }
  if (strcmp(type, "disarm") == 0) {
    forceStop("Operator disarmed", false);
    return;
  }
  if (!operatorArmed || !heartbeatFresh) {
    forceStop("Movement rejected: session not armed/fresh", true);
    return;
  }

  if (strcmp(type, "drive") == 0) {
    if (!document["left"].is<int>() || !document["right"].is<int>()) {
      emitGatewayEvent("error", "BAD_DRIVE", "Missing signed drive percentages");
      return;
    }
    const int left = document["left"].as<int>();
    const int right = document["right"].as<int>();
    const int ttl = document["ttl_ms"] | Protocol::DEFAULT_COMMAND_TTL_MS;
    if (left < -100 || left > 100 || right < -100 || right > 100 ||
        ttl < 1 || ttl > Protocol::MAX_COMMAND_TTL_MS) {
      emitGatewayEvent("error", "BAD_DRIVE_RANGE", "Drive range or TTL rejected");
      return;
    }
    requestedLeftPct = left;
    requestedRightPct = right;
    requestedCommand = Protocol::DriveCommand::DRIVE;
    sendCommand(requestedCommand, requestedLeftPct, requestedRightPct, ttl);
  } else if (strcmp(type, "auto") == 0) {
    requestedCommand = Protocol::DriveCommand::AUTO;
    requestedLeftPct = requestedRightPct = 0;
    sendCommand(requestedCommand, 0, 0, Protocol::DEFAULT_COMMAND_TTL_MS);
  } else if (strcmp(type, "manual") == 0) {
    requestedCommand = Protocol::DriveCommand::MANUAL;
    requestedLeftPct = requestedRightPct = 0;
    sendCommand(requestedCommand, 0, 0, Protocol::DEFAULT_COMMAND_TTL_MS);
  } else {
    emitGatewayEvent("error", "UNKNOWN_COMMAND", "Unknown command type rejected");
  }
}

void serviceSerial() {
  uint16_t budget = 0;
  while (Serial.available() && budget++ < MAX_SERIAL_LINE) {
    const char character = static_cast<char>(Serial.read());
    if (character == '\n' || character == '\r') {
      if (serialLine.length()) {
        handleBrowserLine(serialLine);
        serialLine = "";
      }
    } else if (serialLine.length() < MAX_SERIAL_LINE) {
      serialLine += character;
    } else {
      serialLine = "";
      emitGatewayEvent("error", "INPUT_TOO_LONG", "NDJSON line discarded");
    }
  }
}

bool initializeRadio() {
  if (!radioConfigurationValid()) return false;
  WiFi.mode(WIFI_STA);
  uint8_t actualMac[6]{};
  esp_wifi_get_mac(WIFI_IF_STA, actualMac);
  if (!macEquals(actualMac, Radio::GATEWAY_MAC)) return false;
  if (esp_wifi_set_channel(Radio::CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK) return false;
  if (esp_now_init() != ESP_OK) return false;
  if (esp_now_set_pmk(Radio::PMK) != ESP_OK) return false;
  if (esp_now_register_recv_cb(onRadioReceive) != ESP_OK) return false;
  if (esp_now_register_send_cb(onRadioSend) != ESP_OK) return false;

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, Radio::ROVER_MAC, 6);
  peer.channel = Radio::CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = true;
  memcpy(peer.lmk, Radio::LMK, 16);
  return esp_now_add_peer(&peer) == ESP_OK;
}

void updateWatchdogs() {
  const uint32_t now = millis();
  const bool heartbeatFresh = lastHeartbeatAtMs &&
      now - lastHeartbeatAtMs <= OPERATOR_HEARTBEAT_TIMEOUT_MS;
  const bool roverFresh = lastRoverPacketAtMs &&
      now - lastRoverPacketAtMs <= ROVER_LINK_STALE_MS;
  if (!heartbeatFresh && operatorArmed) {
    forceStop("Operator heartbeat expired", true);
  }
  if (!roverFresh && roverWasFresh && operatorArmed) {
    forceStop("Rover link expired; explicit re-arm required", true);
  }
  if (heartbeatFresh && !operatorWasFresh) {
    emitGatewayEvent("info", "OPERATOR_HEARTBEAT", "Operator heartbeat established");
  }
  operatorWasFresh = heartbeatFresh;
  roverWasFresh = roverFresh;

  if (operatorArmed && now - lastCommandSendAtMs >= Protocol::GATEWAY_COMMAND_INTERVAL_MS) {
    lastCommandSendAtMs = now;
    sendCommand(requestedCommand, requestedLeftPct, requestedRightPct,
                Protocol::DEFAULT_COMMAND_TTL_MS);
  } else if (!operatorArmed && radioReady &&
             now - lastCommandSendAtMs >= Protocol::GATEWAY_COMMAND_INTERVAL_MS) {
    lastCommandSendAtMs = now;
    sendCommand(Protocol::DriveCommand::STOP, 0, 0,
                Protocol::DEFAULT_COMMAND_TTL_MS);
  }
}

String padLcd(String value) {
  if (value.length() > 16) value.remove(16);
  while (value.length() < 16) value += ' ';
  return value;
}

void updateLcd() {
  const uint32_t now = millis();
  if (now - lastLcdAtMs < LCD_INTERVAL_MS) return;
  lastLcdAtMs = now;
  const bool roverFresh = lastRoverPacketAtMs &&
      now - lastRoverPacketAtMs <= ROVER_LINK_STALE_MS;
  String next1;
  String next2;
  if (!radioReady) {
    next1 = "RADIO ERROR";
    next2 = "CHECK CONFIG";
  } else if (!roverFresh) {
    next1 = "CONNECTING...";
    next2 = "WAITING ROVER";
  } else if (!operatorWasFresh) {
    next1 = "ROVER LINK LIVE";
    next2 = "CONNECT DASH";
  } else {
    next1 = operatorArmed ? "MISSION ARMED" : "DASH CONNECTED";
    next2 = operatorArmed ? "ARMED" : "SAFE";
    if (lastRssiDbm != Protocol::RSSI_UNAVAILABLE_DBM) {
      next2 += " RSSI ";
      next2 += lastRssiDbm;
    }
  }
  next1 = padLcd(next1);
  next2 = padLcd(next2);
  if (lcdReady) {
    if (next1 != lcdLine1) {
      lcd.setCursor(0, 0);
      lcd.print(next1);
      lcdLine1 = next1;
    }
    if (next2 != lcdLine2) {
      lcd.setCursor(0, 1);
      lcd.print(next2);
      lcdLine2 = next2;
    }
  }
  digitalWrite(Pin::LED_RED, !radioReady || !roverFresh ? HIGH : LOW);
  digitalWrite(Pin::LED_YELLOW, operatorWasFresh ? HIGH : LOW);
  digitalWrite(Pin::LED_GREEN, radioReady && roverFresh && operatorArmed ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  delay(750);
  Serial.println("DEEPTRACK_GATEWAY_BOOT");
  gatewaySession = esp_random();
  if (gatewaySession == 0) gatewaySession = 1;

  pinMode(Pin::LED_RED, OUTPUT);
  pinMode(Pin::LED_YELLOW, OUTPUT);
  pinMode(Pin::LED_GREEN, OUTPUT);
  digitalWrite(Pin::LED_RED, HIGH);
  digitalWrite(Pin::LED_YELLOW, LOW);
  digitalWrite(Pin::LED_GREEN, LOW);

  Wire.begin(Pin::SDA, Pin::SCL);
  Wire.setClock(100000);
  Wire.beginTransmission(Pin::LCD_ADDRESS);
  lcdReady = Wire.endTransmission() == 0;
  if (lcdReady) {
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DEEPTRACK BOOT");
    lcd.setCursor(0, 1);
    lcd.print("CONNECTING...");
  }
  WiFi.mode(WIFI_STA);
  radioReady = initializeRadio();
  emitHello();
  if (!lcdReady)
    emitGatewayEvent("warning", "LCD_UNAVAILABLE",
                     "Gateway bridge continues without the optional LCD");
  if (!radioReady) emitGatewayEvent("error", "RADIO_DISABLED", "Provision measured MACs and nonzero PMK/LMK keys");
}

void loop() {
  drainRadioPackets();
  serviceSerial();
  updateWatchdogs();
  updateLcd();
  if (millis() - lastHelloAtMs >= HELLO_INTERVAL_MS) {
    lastHelloAtMs = millis();
    emitHello();
  }
}
