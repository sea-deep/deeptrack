import assert from 'node:assert/strict';
import { describe, test } from 'node:test';
import {
  applyGatewayTelemetry,
  DIAGNOSTIC_ACTION,
  formatDiagnosticResult,
  gatewayEventToLog,
  makeGatewayCommand,
  parseGatewayLine,
  PROTOCOL_VERSION,
  shouldAcceptGatewayRecord,
  tofDisplayState,
  upsertScanPoint
} from '../src/lib/state/gatewayProtocol.js';
import { createUnknownTelemetry } from '../src/lib/state/telemetry.js';

describe('gateway NDJSON contract', () => {
  test('requires the matching protocol-v2 wire contract', () => {
    assert.equal(PROTOCOL_VERSION, 2);
  });

  test('builds monotonic session-scoped commands', () => {
    assert.deepEqual(makeGatewayCommand('drive', 42, 7, {
      left: 55, right: -55, ttl_ms: 300
    }), {
      type: 'drive', session: 42, seq: 7,
      left: 55, right: -55, ttl_ms: 300
    });
    assert.throws(() => makeGatewayCommand('drive', 0, 1), RangeError);
  });

  test('rejects non-object records', () => {
    assert.throws(() => parseGatewayLine('[]'), TypeError);
    assert.equal(parseGatewayLine('{"type":"hello"}').type, 'hello');
  });

  test('accepts authenticated rover observations across radio session handover', () => {
    assert.equal(shouldAcceptGatewayRecord({
      type: 'telemetry', source: 'LIVE', session: 41
    }, 42), true);
    assert.equal(shouldAcceptGatewayRecord({
      type: 'scan', source: 'LIVE', session: 41
    }, 42), true);
    assert.equal(shouldAcceptGatewayRecord({
      type: 'event', source: 'rover', session: 41
    }, 42), true);
    assert.equal(shouldAcceptGatewayRecord({
      type: 'ack', source: 'rover', session: 41
    }, 42), true);
    assert.equal(shouldAcceptGatewayRecord({
      type: 'event', source: 'gateway', session: 41
    }, 42), false);
  });

  test('maps explicit nulls to unknown rather than zero', () => {
    const result = applyGatewayTelemetry(createUnknownTelemetry(), {
      type: 'telemetry', source: 'LIVE', temp_c: null,
      humidity_pct: null, gas_raw: 800, gas_state: 'RAW_ONLY',
      water_raw: 100, water_state: 'RAW_ONLY', pitch_deg: null,
      roll_deg: null, heading_deg: null, left_ticks: 2,
      right_ticks: 3, left_raw_ticks: 5, right_raw_ticks: 7,
      left_rejected_debounce_ticks: 1, right_rejected_debounce_ticks: 2,
      left_rejected_state_ticks: 2, right_rejected_state_ticks: 2,
      front_cm: null, front_valid: false,
      front_fresh: false, front_blocked: true, tof_mm: null,
      servo_deg: 90, drive_state: 'SAFE_STOP', status_flags: 4,
      rssi_dbm: null, packet_gaps: 0, reset_reason: 1
    });
    assert.equal(result.temperature, null);
    assert.equal(result.frontDistanceCm, null);
    assert.equal(result.encoderRawL, 5);
    assert.equal(result.encoderRejectedDebounceR, 2);
    assert.equal(result.alertState, 'STOPPED');
    assert.equal(result.source, 'LIVE');
  });

  test('keeps invalid scan evidence but never fabricates a distance', () => {
    const result = upsertScanPoint([], {
      type: 'scan', source: 'LIVE', scan_id: 5, seq: 9,
      angle_deg: 55, distance_mm: null, valid: false,
      range_status: 4
    });
    assert.equal(result[0].valid, false);
    assert.equal(result[0].distance_mm, null);
  });

  test('keeps valid range status without inventing sensor confidence', () => {
    const result = upsertScanPoint([], {
      type: 'scan', source: 'LIVE', scan_id: 6, seq: 10,
      angle_deg: 90, distance_mm: 640, valid: true,
      range_status: 0, confidence_pct: null, timestamp_ms: 1234
    });
    assert.equal(result[0].valid, true);
    assert.equal(result[0].distance_mm, 640);
    assert.equal(result[0].confidence_pct, undefined);
    assert.equal(result[0].timestamp_ms, 1234);
  });

  test('reports centered ToF health independently of a sweep', () => {
    assert.equal(tofDisplayState({ source: 'LIVE', driveState: 'MANUAL', tofMm: 418 }, []), 'LIVE');
    assert.equal(tofDisplayState({ source: 'LIVE', driveState: 'MANUAL', tofMm: null }, []), 'NO RETURN');
    assert.equal(tofDisplayState({ source: 'LIVE', driveState: 'AUTO_SCAN' }, []), 'SCANNING');
    assert.equal(tofDisplayState(
      { source: 'LIVE', driveState: 'AUTO_SCAN' },
      [{ valid: true, distance_mm: 420 }]
    ), 'SCANNING · RETURNS');
  });

  test('treats an obstacle as a directional hold, not a global stop', () => {
    const result = applyGatewayTelemetry(createUnknownTelemetry(), {
      type: 'telemetry', source: 'LIVE', drive_state: 'MANUAL',
      front_cm: 24, ultrasonic_cm: 26, tof_mm: 240,
      front_valid: true, front_fresh: true, front_blocked: true,
      gas_state: 'RAW_ONLY', water_state: 'RAW_ONLY'
    });
    assert.equal(result.alertState, 'ADVISORY');
    assert.equal(result.frontBlocked, true);
  });

  test('renders deterministic rover event labels from uptime', () => {
    const log = gatewayEventToLog({
      source: 'rover', session: 3, seq: 12,
      timestamp_ms: 1234, severity: 2, code: 7, value: 145
    }, 1);
    assert.equal(log.ts, 'T+1.234s');
    assert.equal(log.type, 'ERROR');
    assert.match(log.msg, /STALL_SUSPECTED/);
  });

  test('formats diagnostic results without inventing calibrated units', () => {
    const gas = formatDiagnosticResult({
      type: 'diagnostic', action: DIAGNOSTIC_ACTION.GAS, status: 1,
      value_a: 827, value_b: 805, value_c: 0, value_d: 0,
      timestamp_ms: 1200
    });
    assert.equal(gas.status, 'CHECK');
    assert.match(gas.detail, /827 ADC/);
    assert.match(gas.detail, /baseline not set/);
    assert.doesNotMatch(gas.detail, /ppm|LEL/i);
  });

  test('reports measured ToF and fused dual-sensor front clearance', () => {
    const tof = formatDiagnosticResult({
      type: 'diagnostic', action: DIAGNOSTIC_ACTION.TOF, status: 0,
      value_a: 412, value_b: 0, value_c: 90, value_d: 0
    });
    assert.match(tof.detail, /412 mm at 90°/);
    const front = formatDiagnosticResult({
      type: 'diagnostic', action: DIAGNOSTIC_ACTION.FRONT_RANGE, status: 0,
      value_a: 438, value_b: 412, value_c: 412, value_d: 0
    });
    assert.match(front.detail, /HC-SR04 43\.8 cm/);
    assert.match(front.detail, /VL53L0X 412 mm/);
    assert.match(front.detail, /fused 41\.2 cm/);
  });
});
