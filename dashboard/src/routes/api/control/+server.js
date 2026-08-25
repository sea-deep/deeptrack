import { json } from '@sveltejs/kit';
import { controlCommandSchema } from '$lib/schemas/telemetry.js';

/**
 * POST /api/control
 * Receives motor PWM and emergency commands and formats them for the rover.
 */
export async function POST({ request }) {
  try {
    const body = await request.json();
    const command = controlCommandSchema.parse(body);

    const formattedSerialString = `L:${command.motor_l} R:${command.motor_r}`;

    return json({
      success: true,
      command,
      serialPayload: formattedSerialString,
      mqttTopic: 'rover/control',
      timestamp: new Date().toISOString()
    });
  } catch (err) {
    const errorMsg = err instanceof Error ? err.message : 'Invalid control payload';
    return json({ success: false, error: errorMsg }, { status: 400 });
  }
}
