import { telemetrySchema } from '$lib/schemas/telemetry.js';

/** @type {import('$lib/state/telemetry.js').TelemetryState} */
export const initialTelemetry = {
  timestamp: new Date().toISOString(),
  source: 'SIMULATED',
  temperature: 29.2,
  humidity: 68.4,
  gasRaw: 820,
  gasPinMv: 660,
  gasState: 'SIMULATED',
  waterRaw: 350,
  waterPinMv: 282,
  waterState: 'SIMULATED',
  pitchDeg: 2.1,
  rollDeg: -1.4,
  gyroZ: 0.0,
  encoderL: 420,
  encoderR: 418,
  estimatedDistanceMeters: 4.28,
  frontDistanceCm: 142.0,
  frontValid: true,
  frontFresh: true,
  frontBlocked: false,
  tofMm: 1650,
  headingDeg: 90,
  servoDeg: 90,
  driveState: 'AUTO_ADVANCE',
  statusFlags: 0,
  packetGaps: 0,
  resetReason: 1,
  chassisWidthMm: 220,
  trackWidthMm: 180,
  micrometersPerTick: 5000,
  alertState: 'NOMINAL',
  rssi: null
};

export const mockTelemetry = telemetrySchema.parse(initialTelemetry);

// Explicit demo-only single-point ToF samples for the simulated dashboard mode.
export const initialScanPoints = [
  { angle_deg: 30, distance_mm: 820, valid: true },
  { angle_deg: 40, distance_mm: 950, valid: true },
  { angle_deg: 50, distance_mm: 1100, valid: true },
  { angle_deg: 60, distance_mm: 1350, valid: true },
  { angle_deg: 70, distance_mm: 1580, valid: true },
  { angle_deg: 80, distance_mm: 1620, valid: true },
  { angle_deg: 90, distance_mm: 1650, valid: true }, // Forward opening
  { angle_deg: 100, distance_mm: 1590, valid: true },
  { angle_deg: 110, distance_mm: 1400, valid: true },
  { angle_deg: 120, distance_mm: 1150, valid: true },
  { angle_deg: 130, distance_mm: 980, valid: true },
  { angle_deg: 140, distance_mm: 840, valid: true },
  { angle_deg: 150, distance_mm: 790, valid: true }
];
