# Rover, gateway, and browser protocol

## ESP-NOW

Both peers use `firmware/shared/DeeptrackProtocol.h`. Packets contain a magic
value, protocol version, gateway session, monotonic sequence, message type, and
sender uptime. The encrypted link is unicast to one measured peer MAC; broadcast
control is not accepted. Radio callbacks only copy bounded packets.

The current wire contract is **version 2**. Version 2 adds raw encoder edge
counts, signed accepted ticks, debounce/state rejection counters, and separate
left/right distance-per-tick calibration fields. Rover, gateway, and dashboard
must be upgraded together; mixed version 1/version 2 operation fails closed.

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
  DHT/IMU values, raw/accepted/rejected encoder evidence, drive state,
  calibration, RSSI, reset reason, and gaps;
- `scan`: scan id, sample timestamp, canonical rover-relative bearing encoded
  as `90° = forward` and `>90° = counter-clockwise/left`, nullable distance,
  RangeStatus, validity, and confidence;
- `event`: deterministic gateway/rover safety or state transition;
- `ack`: rover application result for a command sequence.

The browser sends `heartbeat`, `arm`, `disarm`, `manual`, `auto`, and `drive`
records scoped to the hello session and a browser sequence. `drive` uses signed
percentages and a 300 ms TTL. Unscoped `stop` is intentionally accepted and
commands active brake. Pointer/key release stops motion while retaining a fresh
manual arm; blur, page hide, disconnect, and heartbeat expiry also disarm.

For commissioning only, an unscoped
`{"type":"protocol_probe","version":1}` record is accepted while the gateway
is disarmed. It sends only an incompatible packet header. A v2 rover must latch
active brake and print `PROTOCOL_MISMATCH ... SAFE_STOP`; the probe cannot arm
or request motion.

VL53L0X validity and `RangeStatus` are real observations. Confidence is emitted
as `null` because this sensor/API path does not provide a calibrated probability.
`packet_gaps` counts missing global rover sequence numbers plus gateway callback
slot overwrites; ACK/event interleaving is not misreported as telemetry loss.

The timestamp is rover uptime, not laptop wall time. The map interpolates the
dead-reckoning pose in that same rover-uptime domain before casting each ray.
