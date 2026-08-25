import { json } from '@sveltejs/kit';
import { telemetrySchema, rawTelemetrySchema } from '$lib/schemas/telemetry.js';
import { initialTelemetry, initialScanPoints } from '$lib/mocks/telemetryMock.js';
import { supabaseServer } from '$lib/server/supabase.js';

// In-memory state cache for fast streaming access
let currentTelemetry = { ...initialTelemetry };
let currentScanPoints = [...initialScanPoints];

/**
 * GET /api/telemetry
 * Fetches the latest rover state, environmental readings, and LiDAR scan points.
 */
export async function GET() {
  return json({
    success: true,
    telemetry: currentTelemetry,
    scanPoints: currentScanPoints,
    serverTime: new Date().toISOString()
  });
}

/**
 * POST /api/telemetry
 * Ingests a new packet from the ESP32 Gateway / Webhook / MQTT forwarder.
 */
export async function POST({ request }) {
  try {
    const body = await request.json();

    // Check if it's a raw C++ Gateway packet or normalized payload
    if (body.t !== undefined && body.gas !== undefined) {
      const raw = rawTelemetrySchema.parse(body);

      // Compute physical parameters
      const pitch = Math.atan2(-raw.ax, Math.sqrt(raw.ay * raw.ay + raw.az * raw.az)) * (180.0 / Math.PI);
      const roll = Math.atan2(raw.ay, raw.az) * (180.0 / Math.PI);
      const methanePpm = Math.max(0, (raw.gas - 400) * 1.8);
      const methaneLelPercent = +(methanePpm / 500.0).toFixed(2); // 50,000 ppm = 100% LEL

      currentTelemetry = {
        timestamp: new Date().toISOString(),
        temperature: raw.t,
        humidity: raw.h,
        methaneRaw: raw.gas,
        methanePpm: +methanePpm.toFixed(1),
        methaneLelPercent,
        waterRaw: raw.water,
        waterDepthMm: +(raw.water * 0.05).toFixed(1),
        pitchDeg: +pitch.toFixed(1),
        rollDeg: +roll.toFixed(1),
        gyroZ: raw.gz,
        encoderL: currentTelemetry.encoderL,
        encoderR: currentTelemetry.encoderR,
        distanceMeters: currentTelemetry.distanceMeters,
        ultrasonicCm: 150.0,
        dangerState: raw.danger === 1,
        rssi: -65,
        batteryVolts: 5.10
      };
    } else if (body.type === 'scan') {
      // Ingest LiDAR sweep point
      const { angle_deg, distance_mm, valid } = body;
      currentScanPoints = currentScanPoints.map(p => 
        Math.abs(p.angle_deg - angle_deg) < 5 ? { angle_deg, distance_mm, valid } : p
      );
    } else {
      // Full normalized schema validation
      currentTelemetry = telemetrySchema.parse(body);
    }

    // Optional: Log to Supabase telemetry table if available
    try {
      if (supabaseServer) {
        supabaseServer.from('rover_telemetry').insert([{
          timestamp: currentTelemetry.timestamp,
          temperature: currentTelemetry.temperature,
          humidity: currentTelemetry.humidity,
          methane_ppm: currentTelemetry.methanePpm,
          methane_lel_pct: currentTelemetry.methaneLelPercent,
          water_depth_mm: currentTelemetry.waterDepthMm,
          pitch_deg: currentTelemetry.pitchDeg,
          roll_deg: currentTelemetry.rollDeg,
          danger_state: currentTelemetry.dangerState
        }]).then(() => {});
      }
    } catch (e) {}

    return json({ success: true, telemetry: currentTelemetry });
  } catch (err) {
    const errorMsg = err instanceof Error ? err.message : 'Invalid telemetry payload';
    return json({ success: false, error: errorMsg }, { status: 400 });
  }
}
