# Rover, gateway, and browser protocol

## ESP-NOW

Both peers use `firmware/shared/DeeptrackProtocol.h`. Packets contain a magic
value, protocol version, gateway session, monotonic sequence, message type, and
sender uptime. The encrypted link is unicast to one measured peer MAC; broadcast
control is not accepted. Radio callbacks only copy bounded packets.

The rover uses local receipt time for TTL enforcement because peer clocks are
not synchronized. Normal TTL is 300 ms and the hard maximum is 500 ms. A new
gateway session brakes and starts disarmed. Safety STOP is always processed;
movement requires current session, increasing sequence, fresh TTL, arm state,
and local rover safety gates.

The gateway also disarms on a transition from fresh to stale rover traffic and
continues sending STOP. Reappearance of the radio peer never resumes the prior
manual vector or autonomous state; the operator must explicitly arm again.

## USB NDJSON

The gateway emits one JSON object per line:

- `hello`: protocol, gateway session/identity, local STA MAC, radio, LCD, and arm state;
- `telemetry`: explicit validity/freshness, raw environmental ADC/mV, nullable
  DHT/IMU values, ticks, drive state, calibration, RSSI, reset reason, and gaps;
- `scan`: scan id, angle, nullable distance, RangeStatus, validity, confidence;
- `event`: deterministic gateway/rover safety or state transition;
- `ack`: rover application result for a command sequence.

The browser sends `heartbeat`, `arm`, `disarm`, `manual`, `auto`, and `drive`
records scoped to the hello session and a browser sequence. `drive` uses signed
percentages and a 300 ms TTL. Unscoped `stop` is intentionally accepted and
commands active brake. Pointer/key release stops motion while retaining a fresh
manual arm; blur, page hide, disconnect, and heartbeat expiry also disarm.

VL53L0X validity and `RangeStatus` are real observations. Confidence is emitted
as `null` because this sensor/API path does not provide a calibrated probability.
`packet_gaps` counts missing global rover sequence numbers plus gateway callback
slot overwrites; ACK/event interleaving is not misreported as telemetry loss.
