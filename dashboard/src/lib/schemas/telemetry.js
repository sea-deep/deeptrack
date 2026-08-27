import { z } from 'zod';

export const telemetrySourceSchema = z.enum(['UNKNOWN', 'SIMULATED', 'LIVE', 'STALE']);
export const gasStateSchema = z.enum([
  'UNKNOWN',
  'SIMULATED',
  'WARMING',
  'RAW_ONLY',
  'QUALITATIVE_READY',
  'QUALITATIVE_ADVISORY',
  'SENSOR_FAULT'
]);
export const waterStateSchema = z.enum(['UNKNOWN', 'SIMULATED', 'RAW_ONLY', 'DRY_BASELINE', 'CONTACT', 'SENSOR_FAULT']);
export const alertStateSchema = z.enum(['NOMINAL', 'ADVISORY', 'STOPPED', 'UNKNOWN']);

// Normalized Application Telemetry State for Mission Control
export const telemetrySchema = z.object({
  timestamp: z.string().datetime(),
  source: telemetrySourceSchema,
  temperature: z.number().nullable().describe('DHT22 Temp in °C; null until observed'),
  humidity: z.number().nullable().describe('DHT22 Humidity in %; null until observed'),
  gasRaw: z.number().int().nullable().describe('Uncalibrated MQ-4 raw ADC (0-4095); null until observed'),
  gasPinMv: z.number().int().nullable().default(null),
  gasState: gasStateSchema,
  waterRaw: z.number().int().nullable().describe('Water sensor ADC (0-4095); null until observed'),
  waterPinMv: z.number().int().nullable().default(null),
  waterState: waterStateSchema,
  pitchDeg: z.number().nullable().describe('Computed Pitch angle from MPU6050 (deg); null until observed'),
  rollDeg: z.number().nullable().describe('Computed Roll angle from MPU6050 (deg); null until observed'),
  gyroZ: z.number().nullable().describe('Yaw rate (deg/s); null until observed'),
  encoderL: z.number().int().nullable().describe('Left LM393 Optical Encoder Pulse Count; null until observed'),
  encoderR: z.number().int().nullable().describe('Right LM393 Optical Encoder Pulse Count; null until observed'),
  encoderRawL: z.number().int().nullable().default(null),
  encoderRawR: z.number().int().nullable().default(null),
  encoderRejectedDebounceL: z.number().int().nullable().default(null),
  encoderRejectedDebounceR: z.number().int().nullable().default(null),
  encoderRejectedStateL: z.number().int().nullable().default(null),
  encoderRejectedStateR: z.number().int().nullable().default(null),
  estimatedDistanceMeters: z.number().nullable().describe('Uncertainty-bearing distance estimate; null until calibrated observations exist'),
  frontDistanceCm: z.number().nullable().describe('HC-SR04 forward proximity in cm; null until observed'),
  ultrasonicDistanceCm: z.number().nullable().default(null),
  frontValid: z.boolean().nullable().default(null),
  frontFresh: z.boolean().nullable().default(null),
  frontBlocked: z.boolean().nullable().default(null),
  tofMm: z.number().nullable().default(null),
  headingDeg: z.number().nullable().default(null),
  servoDeg: z.number().nullable().default(null),
  driveState: z.string().default('UNKNOWN'),
  statusFlags: z.number().int().nullable().default(null),
  packetGaps: z.number().int().nullable().default(null),
  resetReason: z.number().int().nullable().default(null),
  chassisWidthMm: z.number().int().nullable().default(null),
  trackWidthMm: z.number().int().nullable().default(null),
  micrometersPerTick: z.number().int().nullable().default(null),
  alertState: alertStateSchema,
  rssi: z.number().nullable().describe('ESP-NOW RSSI in dBm; null when unavailable')
});
