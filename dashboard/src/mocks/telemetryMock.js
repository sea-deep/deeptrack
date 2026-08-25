import { telemetrySchema } from '$lib/schemas/telemetry.js';

export const initialTelemetry = {
  timestamp: new Date().toISOString(),
  temperature: 29.2, // Typical underground coal mine temp (DGMS limit 33.5°C wet bulb)
  humidity: 68.4,
  methaneRaw: 820,
  methanePpm: 450, // 450 ppm CH4
  methaneLelPercent: 0.9, // 0.9% LEL (DGMS threshold: 1.0% = Action, 2.0% = Evacuate)
  waterRaw: 350,
  waterDepthMm: 12.5, // 12.5 mm pooled water
  pitchDeg: 2.1,
  rollDeg: -1.4,
  gyroZ: 0.0,
  encoderL: 420,
  encoderR: 418,
  distanceMeters: 4.28, // 420 pulses * (pi * 0.065m / 20) = 4.28m
  ultrasonicCm: 142.0,
  dangerState: false,
  rssi: -64,
  batteryVolts: 5.12
};

export const mockTelemetry = telemetrySchema.parse(initialTelemetry);

// Initial LiDAR radar sweep point cloud (30° to 150° in 10° steps)
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
