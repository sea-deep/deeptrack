export const PROTOCOL_VERSION = 2;
export const HEARTBEAT_INTERVAL_MS = 150;
export const DRIVE_REFRESH_MS = 100;
export const COMMAND_TTL_MS = 300;
export const TELEMETRY_STALE_MS = 500;

export const DIAGNOSTIC_ACTION = Object.freeze({
  STATUS: 1, I2C_SCAN: 2, DHT: 3, IMU: 4, GAS: 5, WATER: 6,
  FRONT_RANGE: 7, TOF: 8, ENCODERS: 9, START_SCAN: 10,
  SERVO: 11, LEDS: 12, BUZZER: 13, RADIO_LINK: 14,
  MOTOR_STATE: 15, SENSOR_FRESHNESS: 16, CALIBRATION_STATUS: 17,
  CALIBRATE_IMU: 18
});

/** @type {Readonly<Record<number, string>>} */
const diagnosticNames = Object.freeze({
  1: 'System status', 2: 'I2C bus scan', 3: 'Climate sensor',
  4: 'IMU', 5: 'MQ-4 signal', 6: 'Water probe', 7: 'Front range',
  8: 'VL53L0X', 9: 'Encoder snapshot', 10: 'Seven-angle scan',
  11: 'Scanner servo', 12: 'Status LEDs', 13: 'Buzzer',
  14: 'ESP-NOW link', 15: 'Motor state', 16: 'Sensor freshness',
  17: 'Calibration readiness', 18: 'IMU calibration'
});

/** @param {Record<string, any>} packet */
export function formatDiagnosticResult(packet) {
  if (packet.type !== 'diagnostic' || !Number.isInteger(packet.action))
    throw new TypeError('Not a diagnostic result');
  const status = ['PASS', 'CHECK', 'FAIL', 'REJECTED'][packet.status] || 'UNKNOWN';
  const a = Number(packet.value_a) || 0;
  const b = Number(packet.value_b) || 0;
  const c = Number(packet.value_c) || 0;
  const d = Number(packet.value_d) || 0;
  let detail = `values: ${a}, ${b}, ${c}, ${d}`;
  switch (packet.action) {
    case DIAGNOSTIC_ACTION.STATUS:
      detail = `front ${b < 0 ? 'unavailable' : (b / 10).toFixed(1) + ' cm'} · ticks L ${c} / R ${d}`;
      break;
    case DIAGNOSTIC_ACTION.I2C_SCAN:
      detail = `${a} device${a === 1 ? '' : 's'} found · VL53L0X ${(packet.flags & (1 << 9)) ? 'found' : 'missing'} · MPU6050 ${(packet.flags & (1 << 10)) ? 'found' : 'missing'}`;
      break;
    case DIAGNOSTIC_ACTION.DHT:
      detail = `${(a / 100).toFixed(1)} °C · ${(b / 100).toFixed(1)}% RH`;
      break;
    case DIAGNOSTIC_ACTION.IMU:
      detail = `pitch ${(a / 100).toFixed(2)}° · roll ${(b / 100).toFixed(2)}° · heading ${(c / 100).toFixed(2)}°`;
      break;
    case DIAGNOSTIC_ACTION.GAS:
      detail = `raw ${a} ADC · pin ${b} mV · baseline ${c || 'not set'}`;
      break;
    case DIAGNOSTIC_ACTION.WATER:
      detail = `raw ${a} ADC · pin ${b} mV · dry ${c || 'not set'} · wet ${d || 'not set'}`;
      break;
    case DIAGNOSTIC_ACTION.FRONT_RANGE:
      detail = `HC-SR04 ${a < 0 ? 'unavailable' : (a / 10).toFixed(1) + ' cm'} · VL53L0X ${b < 0 ? 'unavailable' : b + ' mm'} · fused ${c < 0 ? 'unavailable' : (c / 10).toFixed(1) + ' cm'} · ${d ? 'BLOCKED' : 'clear'}`;
      break;
    case DIAGNOSTIC_ACTION.TOF:
      detail = a < 0 ? `no valid return · range status ${b}` : `${a} mm at ${c}° · range status ${b}`;
      break;
    case DIAGNOSTIC_ACTION.ENCODERS:
      detail = `raw ticks L ${a} / R ${b} · accepted signed ticks L ${c} / R ${d}`;
      break;
    case DIAGNOSTIC_ACTION.START_SCAN:
      detail = status === 'PASS' ? `stationary scan ${a} started` : 'requires a stationary rover with healthy servo and ToF';
      break;
    case DIAGNOSTIC_ACTION.SERVO:
      detail = status === 'PASS'
        ? `commanded ${a}° · library ${b}° · GPIO pulse ${c} µs`
        : status === 'FAIL'
          ? `commanded ${a}°, but no valid PWM pulse was measured on the servo pin`
          : 'servo test rejected';
      break;
    case DIAGNOSTIC_ACTION.LEDS:
      detail = status === 'PASS' ? 'red and green flashed for 0.5 s' : 'LED test rejected';
      break;
    case DIAGNOSTIC_ACTION.BUZZER:
      detail = status === 'PASS' ? (a ? '0.5 s pulse started' : 'off') : 'buzzer test rejected';
      break;
    case DIAGNOSTIC_ACTION.RADIO_LINK:
      detail = `${a === -999 ? 'RSSI unavailable' : `gateway RSSI ${a} dBm`} · last valid command ${b < 0 ? 'never' : b + ' ms ago'} · radio ${c ? 'ready' : 'not ready'}`;
      break;
    case DIAGNOSTIC_ACTION.MOTOR_STATE:
      detail = `target L ${a} / R ${b} · applied L ${c} / R ${d}`;
      break;
    case DIAGNOSTIC_ACTION.SENSOR_FRESHNESS:
      detail = `HC-SR04 ${a < 0 ? 'never' : a + ' ms'} · DHT ${b < 0 ? 'never' : b + ' ms'} · IMU ${c < 0 ? 'never' : c + ' ms'} · centered ToF ${d < 0 ? 'never' : d + ' ms'}`;
      break;
    case DIAGNOSTIC_ACTION.CALIBRATION_STATUS:
      detail = `chassis ${a || 'unset'} mm · track ${b || 'unset'} mm · ${c || 'unset'} µm/tick · turn-90 ${d || 'unset'} ms`;
      break;
    case DIAGNOSTIC_ACTION.CALIBRATE_IMU:
      detail = status === 'PASS'
        ? `level recorded · gyro bias sampling for ${(a / 1000).toFixed(1)} s · keep rover still`
        : 'requires a disarmed, stationary rover with fresh IMU data';
      break;
  }
  return {
    label: diagnosticNames[packet.action] || `Diagnostic ${packet.action}`,
    status,
    detail,
    timestampMs: Number.isFinite(packet.timestamp_ms) ? packet.timestamp_ms : null
  };
}

/** @type {Record<number, string>} */
const roverEventNames = {
  1: 'BOOT', 2: 'FRONT_BLOCKED', 3: 'FRONT_STALE',
  4: 'COMMAND_EXPIRED', 5: 'COMMAND_REJECTED', 6: 'ACTIVE_BRAKE',
  7: 'STALL_SUSPECTED', 8: 'WATER_CONTACT', 9: 'AUTO_STUCK',
  10: 'LINK_RESTORED', 11: 'TILT_STOP', 12: 'GAS_ADVISORY',
  13: 'CALIBRATION_CHANGED', 14: 'AUTO_STARTED',
  15: 'AUTO_TURN_COMPLETE'
};

/** @param {string} line @returns {Record<string, any>} */
export function parseGatewayLine(line) {
  const value = JSON.parse(line);
  if (!value || Array.isArray(value) || typeof value !== 'object' ||
      typeof value.type !== 'string') {
    throw new TypeError('Gateway record must be an object with a type');
  }
  return value;
}

/** @param {string} type @param {number} session @param {number} sequence @param {Record<string, unknown>} payload */
export function makeGatewayCommand(type, session, sequence, payload = {}) {
  if (!Number.isInteger(session) || session <= 0) {
    throw new RangeError('A positive gateway session is required');
  }
  if (!Number.isInteger(sequence) || sequence <= 0) {
    throw new RangeError('A positive command sequence is required');
  }
  return { type, session, seq: sequence, ...payload };
}

/**
 * Rover-originated observations carry the rover's active radio session. During
 * boot/session handover that value can briefly differ from the gateway's USB
 * control session. The ESP-NOW gateway has already authenticated the rover MAC
 * and encrypted peer, so observation records must not be discarded for that
 * transient mismatch. Gateway ACKs/events remain session-scoped.
 * @param {Record<string, any>} packet
 * @param {number | null} gatewaySession
 */
export function shouldAcceptGatewayRecord(packet, gatewaySession) {
  if (!gatewaySession || !Number.isInteger(packet.session)) return true;
  if (packet.type === 'telemetry' || packet.type === 'scan' ||
      packet.source === 'rover') return true;
  return packet.session === gatewaySession;
}

/** @param {unknown} value */
const finiteOrNull = (value) =>
  typeof value === 'number' && Number.isFinite(value) ? value : null;

/** @param {Record<string, any>} previous @param {Record<string, any>} packet @returns {any} */
export function applyGatewayTelemetry(previous, packet) {
  if (packet.type !== 'telemetry' || packet.source !== 'LIVE') {
    throw new TypeError('Not a live telemetry record');
  }
  const frontBlocked = packet.front_blocked === true;
  const stopped = packet.drive_state === 'SAFE_STOP' ||
                  packet.drive_state === 'STUCK';
  const advisory = packet.gas_state === 'QUALITATIVE_ADVISORY' ||
                   packet.water_state === 'CONTACT';
  return {
    ...previous,
    timestamp: new Date().toISOString(),
    source: 'LIVE',
    temperature: finiteOrNull(packet.temp_c),
    humidity: finiteOrNull(packet.humidity_pct),
    gasRaw: finiteOrNull(packet.gas_raw),
    gasPinMv: finiteOrNull(packet.gas_pin_mv),
    gasState: typeof packet.gas_state === 'string' ? packet.gas_state : 'UNKNOWN',
    waterRaw: finiteOrNull(packet.water_raw),
    waterPinMv: finiteOrNull(packet.water_pin_mv),
    waterState: typeof packet.water_state === 'string' ? packet.water_state : 'UNKNOWN',
    pitchDeg: finiteOrNull(packet.pitch_deg),
    rollDeg: finiteOrNull(packet.roll_deg),
    headingDeg: finiteOrNull(packet.heading_deg),
    encoderL: finiteOrNull(packet.left_ticks),
    encoderR: finiteOrNull(packet.right_ticks),
    encoderRawL: finiteOrNull(packet.left_raw_ticks),
    encoderRawR: finiteOrNull(packet.right_raw_ticks),
    encoderRejectedDebounceL: finiteOrNull(packet.left_rejected_debounce_ticks),
    encoderRejectedDebounceR: finiteOrNull(packet.right_rejected_debounce_ticks),
    encoderRejectedStateL: finiteOrNull(packet.left_rejected_state_ticks),
    encoderRejectedStateR: finiteOrNull(packet.right_rejected_state_ticks),
    frontDistanceCm: finiteOrNull(packet.front_cm),
    ultrasonicDistanceCm: finiteOrNull(packet.ultrasonic_cm),
    frontValid: packet.front_valid === true,
    frontFresh: packet.front_fresh === true,
    frontBlocked,
    tofMm: finiteOrNull(packet.tof_mm),
    servoDeg: finiteOrNull(packet.servo_deg),
    driveState: typeof packet.drive_state === 'string' ? packet.drive_state : 'UNKNOWN',
    statusFlags: finiteOrNull(packet.status_flags),
    rssi: finiteOrNull(packet.rssi_dbm),
    packetGaps: finiteOrNull(packet.packet_gaps),
    resetReason: finiteOrNull(packet.reset_reason),
    chassisWidthMm: finiteOrNull(packet.chassis_width_mm),
    trackWidthMm: finiteOrNull(packet.track_width_mm),
    micrometersPerTick: finiteOrNull(packet.micrometers_per_tick),
    // A front obstacle is a directional motion hold: forward is blocked while
    // reverse and pivot remain available. Reserve STOPPED for actual rover
    // SAFE_STOP/STUCK states so the console never implies a disarm that did
    // not happen.
    alertState: stopped ? 'STOPPED' : frontBlocked || advisory ? 'ADVISORY' : 'NOMINAL'
  };
}

/** @param {Array<Record<string, any>>} points @param {Record<string, any>} packet @returns {any} */
export function upsertScanPoint(points, packet) {
  if (packet.type !== 'scan' || packet.source !== 'LIVE' ||
      !Number.isFinite(packet.angle_deg)) return points;
  const next = {
    angle_deg: packet.angle_deg,
    distance_mm: Number.isFinite(packet.distance_mm) ? packet.distance_mm : null,
    valid: packet.valid === true && Number.isFinite(packet.distance_mm),
    seq: Number.isFinite(packet.seq) ? packet.seq : undefined,
    scan_id: Number.isFinite(packet.scan_id) ? packet.scan_id : undefined,
    range_status: Number.isFinite(packet.range_status) ? packet.range_status : undefined,
    confidence_pct: Number.isFinite(packet.confidence_pct) ? packet.confidence_pct : undefined,
    timestamp_ms: Number.isFinite(packet.timestamp_ms) ? packet.timestamp_ms : undefined
  };
  const index = points.findIndex((point) =>
    point.scan_id === next.scan_id &&
    Math.abs(point.angle_deg - next.angle_deg) < 0.01);
  if (index < 0) return [...points.slice(-48), next];
  return points.map((point, i) => i === index ? next : point);
}

/** @param {Record<string, any>} telemetry @param {Array<Record<string, any>>} scanPoints */
export function tofDisplayState(telemetry, scanPoints) {
  if (telemetry.source !== 'LIVE') return 'UNKNOWN';
  if (telemetry.driveState === 'AUTO_SCAN') {
    return scanPoints.some((point) => point.valid === true)
      ? 'SCANNING · RETURNS'
      : 'SCANNING';
  }
  return Number.isFinite(telemetry.tofMm) ? 'LIVE' : 'NO RETURN';
}

/** @param {unknown} value */
function severityName(value) {
  if (value === 2 || value === 'error') return 'ERROR';
  if (value === 1 || value === 'warning') return 'WARNING';
  return 'INFO';
}

/** @param {Record<string, any>} packet @param {number} fallbackId */
export function gatewayEventToLog(packet, fallbackId) {
  const code = typeof packet.code === 'number'
    ? (roverEventNames[packet.code] || 'ROVER_EVENT_' + packet.code)
    : (packet.code || 'EVENT');
  const source = packet.source === 'rover' ? 'Rover' : 'Gateway';
  const uptime = Number.isFinite(packet.timestamp_ms)
    ? 'T+' + (packet.timestamp_ms / 1000).toFixed(3) + 's'
    : 'T+UNKNOWN';
  const valueSuffix = Number.isFinite(packet.value)
    ? ' · value ' + packet.value : '';
  return {
    id: [packet.source || 'gateway', packet.session || 0,
      packet.seq || fallbackId].join('-'),
    ts: uptime,
    type: severityName(packet.severity),
    source,
    icon: packet.source === 'rover' ? 'memory' : 'router',
    msg: packet.message || code + valueSuffix
  };
}
