/**
 * @typedef {Object} TelemetryState
 * @property {string} timestamp
 * @property {'UNKNOWN' | 'SIMULATED' | 'LIVE' | 'STALE'} source
 * @property {number | null} temperature
 * @property {number | null} humidity
 * @property {number | null} gasRaw
 * @property {number | null} gasPinMv
 * @property {'UNKNOWN' | 'SIMULATED' | 'WARMING' | 'RAW_ONLY' | 'QUALITATIVE_READY' | 'QUALITATIVE_ADVISORY' | 'SENSOR_FAULT'} gasState
 * @property {number | null} waterRaw
 * @property {number | null} waterPinMv
 * @property {'UNKNOWN' | 'SIMULATED' | 'RAW_ONLY' | 'DRY_BASELINE' | 'CONTACT' | 'SENSOR_FAULT'} waterState
 * @property {number | null} pitchDeg
 * @property {number | null} rollDeg
 * @property {number | null} gyroZ
 * @property {number | null} encoderL
 * @property {number | null} encoderR
 * @property {number | null} estimatedDistanceMeters
 * @property {number | null} frontDistanceCm
 * @property {boolean | null} frontValid
 * @property {boolean | null} frontFresh
 * @property {boolean | null} frontBlocked
 * @property {number | null} tofMm
 * @property {number | null} headingDeg
 * @property {number | null} servoDeg
 * @property {string} driveState
 * @property {number | null} statusFlags
 * @property {number | null} packetGaps
 * @property {number | null} resetReason
 * @property {number | null} chassisWidthMm
 * @property {number | null} trackWidthMm
 * @property {number | null} micrometersPerTick
 * @property {'NOMINAL' | 'ADVISORY' | 'STOPPED' | 'UNKNOWN'} alertState
 * @property {number | null} rssi
 */

/**
 * @typedef {Object} TelemetryHistory
 * @property {number[]} gas
 * @property {number[]} temperature
 * @property {number[]} water
 * @property {number[]} ultrasonic
 * @property {number[]} distance
 * @property {number[]} pitch
 * @property {number[]} roll
 */

/** @returns {TelemetryState} */
export function createUnknownTelemetry() {
  return {
    timestamp: new Date().toISOString(),
    source: 'UNKNOWN',
    temperature: null,
    humidity: null,
    gasRaw: null,
    gasPinMv: null,
    gasState: 'UNKNOWN',
    waterRaw: null,
    waterPinMv: null,
    waterState: 'UNKNOWN',
    pitchDeg: null,
    rollDeg: null,
    gyroZ: null,
    encoderL: null,
    encoderR: null,
    estimatedDistanceMeters: null,
    frontDistanceCm: null,
    frontValid: null,
    frontFresh: null,
    frontBlocked: null,
    tofMm: null,
    headingDeg: null,
    servoDeg: null,
    driveState: 'UNKNOWN',
    statusFlags: null,
    packetGaps: null,
    resetReason: null,
    chassisWidthMm: null,
    trackWidthMm: null,
    micrometersPerTick: null,
    alertState: 'UNKNOWN',
    rssi: null
  };
}

/** @returns {TelemetryHistory} */
export function createEmptyHistory() {
  return {
    gas: [],
    temperature: [],
    water: [],
    ultrasonic: [],
    distance: [],
    pitch: [],
    roll: []
  };
}

/** @returns {TelemetryHistory} */
export function createDemoHistory() {
  return {
    gas: [800, 810, 815, 820, 825, 830, 820, 815, 820, 825, 820, 830, 825, 820],
    temperature: [28.8, 28.9, 29.0, 29.1, 29.2, 29.2, 29.3, 29.2, 29.1, 29.2, 29.2],
    water: [340, 345, 350, 350, 352, 348, 350, 355, 350, 350],
    ultrasonic: [145, 144, 143, 142, 142, 141, 142, 142, 143, 142],
    distance: [0.0, 0.4, 0.8, 1.2, 1.8, 2.4, 3.1, 3.8, 4.28],
    pitch: [0.5, 0.8, 1.2, 1.5, 1.8, 2.1, 2.0, 2.1, 2.1],
    roll: [-0.4, -0.6, -0.9, -1.2, -1.4, -1.3, -1.4, -1.4]
  };
}
