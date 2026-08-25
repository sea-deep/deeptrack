import { z } from 'zod';

// Operating Modes for Coal Mine Rover
export const operatingModeSchema = z.enum(['MANUAL', 'AUTO_EXPLORE', 'AUTO_NAVIGATE']);

// Raw Telemetry JSON emitted by ESP32 Gateway (matches TelemetryPacket)
export const rawTelemetrySchema = z.object({
  t: z.number().describe('Temperature in °C from DHT22'),
  h: z.number().describe('Relative Humidity % from DHT22'),
  ax: z.number().describe('Accelerometer X (g) from MPU6050'),
  ay: z.number().describe('Accelerometer Y (g) from MPU6050'),
  az: z.number().describe('Accelerometer Z (g) from MPU6050'),
  gx: z.number().describe('Gyroscope X (deg/s) from MPU6050'),
  gy: z.number().describe('Gyroscope Y (deg/s) from MPU6050'),
  gz: z.number().describe('Gyroscope Z (deg/s) from MPU6050'),
  gas: z.number().int().describe('Raw ADC reading (0-4095) from MQ-4 Methane Sensor'),
  water: z.number().int().describe('Raw ADC reading (0-4095) from Submersible Water Sensor'),
  danger: z.number().int().describe('Danger state: 0=nominal, 1=hazard/alarm')
});

// LiDAR ToF Radar Scan Packet emitted by ESP32 (matches ScanPacket)
export const scanPacketSchema = z.object({
  type: z.literal('scan').default('scan'),
  seq: z.number().int().describe('Scan sequence index'),
  angle_deg: z.number().describe('SG90 Servo Sweep Angle (30° - 150°)'),
  distance_mm: z.number().describe('VL53L0X Laser ToF Target Distance in mm'),
  valid: z.boolean().describe('True if distance reading is within valid range (30-2000mm)'),
  timestamp_ms: z.number().int().optional().describe('Microcontroller timestamp in ms')
});

// Rover Teleoperation & Mission Control Command
export const controlCommandSchema = z.object({
  mode: operatingModeSchema.default('MANUAL'),
  motor_l: z.number().int().min(-255).max(255).default(0).describe('Left TT motor PWM target (-255 to 255)'),
  motor_r: z.number().int().min(-255).max(255).default(0).describe('Right TT motor PWM target (-255 to 255)'),
  estop: z.boolean().default(false).describe('Immediate hardware brake trigger'),
  isRecordingMap: z.boolean().default(false).describe('Toggle continuous 2D SLAM recording'),
  waypointTarget: z.object({ x: z.number(), y: z.number() }).nullable().optional(),
  lights: z.boolean().optional().describe('Toggle front LED spotlight array'),
  buzzer: z.boolean().optional().describe('Trigger audible beacon buzzer')
});

// Normalized Application Telemetry State for Mission Control
export const telemetrySchema = z.object({
  timestamp: z.string().datetime(),
  temperature: z.number().describe('DHT22 Temp in °C'),
  humidity: z.number().describe('DHT22 Humidity in %'),
  methaneRaw: z.number().int().describe('MQ-4 Raw ADC (0-4095)'),
  methanePpm: z.number().describe('Calculated Methane concentration in ppm'),
  methaneLelPercent: z.number().describe('% LEL (Lower Explosive Limit: 50,000 ppm = 100% LEL)'),
  waterRaw: z.number().int().describe('Water sensor ADC (0-4095)'),
  waterDepthMm: z.number().describe('Calibrated flood depth in mm'),
  pitchDeg: z.number().describe('Computed Pitch angle from MPU6050 (deg)'),
  rollDeg: z.number().describe('Computed Roll angle from MPU6050 (deg)'),
  gyroZ: z.number().describe('Yaw rate (deg/s)'),
  encoderL: z.number().int().describe('Left LM393 Optical Encoder Pulse Count'),
  encoderR: z.number().int().describe('Right LM393 Optical Encoder Pulse Count'),
  distanceMeters: z.number().describe('Distance Traversed based on 65mm Wheel Diameter & 20-slot disc'),
  ultrasonicCm: z.number().describe('HC-SR04 Forward Proximity in cm'),
  dangerState: z.boolean().describe('Active DGMS Coal Mine Hazard Alarm'),
  rssi: z.number().describe('Wireless Gateway Link Quality (dBm)'),
  batteryVolts: z.number().describe('Power Bank / Supply Voltage')
});
