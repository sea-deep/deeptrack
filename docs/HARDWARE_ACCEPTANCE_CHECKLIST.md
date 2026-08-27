# DEEPTRACK integrated hardware acceptance

Nothing in this checklist is passed by compilation, simulation, or visual
inspection. Record measurements in `docs/CALIBRATION.md` and outcomes in
`docs/TEST_RESULTS.md`. Keep the physical motor-power switch reachable.

## 1. Electrical and diagnostic gate

1. With power removed, confirm wiring and divider values against
   `docs/WIRING_GUIDE.md`.
2. Power logic without motor VM. Measure 5 V, 3V3, both TB6612 VCC/STBY pins,
   ground continuity, and maximum HC-SR04/MQ-4 divider outputs.
3. Upload `rover-diagnostics`; test I2C identities, DHT, raw gas, raw water,
   encoders, servo travel, ultrasonic, and ToF one component at a time.
4. With wheels lifted, use bounded diagnostic motor commands to confirm the
   physical front and inversion flags. Do not use the diagnostic sketch's
   legacy gas/water thresholds as mission calibration.
5. Upload `gateway-diagnostics`; verify LEDs and every LCD character position
   on ESP32 despite the library compatibility warning.

## 2. Mission image and radio provisioning

```bash
./scripts/firmware/compile.sh all
./scripts/firmware/upload.sh rover-mission /dev/ttyUSB_ROVER
./scripts/firmware/monitor.sh /dev/ttyUSB_ROVER
```

Record `ROVER_STA_MAC`. Upload `gateway-mission`, connect the real dashboard,
and record `gateway_sta_mac` from the hello. Then:

```bash
./scripts/firmware/provision-radio.sh ROVER_STA_MAC GATEWAY_STA_MAC CHANNEL
./scripts/firmware/upload.sh rover-mission /dev/ttyUSB_ROVER
./scripts/firmware/upload.sh gateway-mission /dev/ttyUSB_GATEWAY
```

The local config is ignored and mode `0600`. Verify neither mission image arms
with a missing config, zero keys, swapped MACs, a different local MAC, or the
wrong channel.

## 3. Calibration gate

- Record chassis width, driven-wheel track width, distance per encoder tick,
  and repeated low-duty 90-degree turn timing.
- Record dry and clean-water probe points. Confirm at least 100 ADC counts of
  separation; otherwise water state must remain raw-only.
- Complete MQ-4 conditioning and safe clean-air baseline procedure. Record raw
  median/spread and issue `cal gasbase` only when evidence is acceptable.
- Measure HC-SR04/ToF errors at multiple ranges and surfaces.
- Verify servo center and 35°/145° cable clearance.
- Record motor inversion with `cal motors L R`, then record servo center and
  whether lower angles point left or right with `cal servo CENTER DIRECTION`.
- Measure stopping distance at each command percent, surface, and battery state.
  Adjust centralized stop/clear values only from evidence.

## 4. Safety behavior

| ID | Test | Required result | Status |
|---|---|---|---|
| INT-01 | Boot/reset with motor VM enabled | Active brake; no startup motion | NOT RUN |
| INT-02 | Front target crosses stop/clear bands | 24/32 cm starting hysteresis, then measured replacement values | NOT RUN |
| INT-03 | Disconnect HC-SR04 ECHO | Forward blocked after no valid sample; no zero-as-clear | NOT RUN |
| INT-04 | Hold forward then release key/pointer | Immediate active brake; no stale vector resumes | NOT RUN |
| INT-05 | Hold drive then blur/hide tab | STOP then disarm within watchdog/TTL envelope | NOT RUN |
| INT-06 | Unplug gateway USB or interrupt rover RF while moving | Gateway and rover stop; restored RF remains disarmed until explicit re-arm | NOT RUN |
| INT-07 | Power/reset either ESP32 while moving | Motion stops and new session starts disarmed | NOT RUN |
| INT-08 | Block each driven side separately at low safe power | Side-specific stall event (`1` left, `2` right) and active brake within calibrated limit | NOT RUN |
| INT-09 | Exceed tilt threshold on restrained bench | Persistent tilt event, disarm, active brake | NOT RUN |
| INT-10 | Apply and remove calibrated clean-water contact | 3-sample contact and 5-sample hysteretic clear; contact disarms and actively brakes | NOT RUN |

## 5. Autonomous stationary scan

Use barriers that cannot trap or damage the rover.

1. Confirm auto is rejected before width and turn calibration.
2. Confirm the rover brakes before moving the servo.
3. Present invalid/out-of-range ToF targets; those sectors must not be selected.
4. Present one wide-looking ray without valid neighbors; it must not be selected.
5. Present a measured opening wider than chassis width plus both margins and
   optical uncertainty; record the chosen direction.
6. Confirm only a low-duty timed pivot occurs, then scanner centering, settling,
   and a fresh HC-SR04 reading before advance.
7. Block every opening. Required state is `STUCK`; autonomous reverse must never
   occur because there is no rear clearance sensor.

## 6. Dashboard and estimated map

- Demo mode: complete simulated judge flow and zero USB connection controls.
- Real mode: observations begin unknown; USB, radio, rover-link, armed, stale,
  blocked, rejected, and stuck states remain distinguishable.
- Real controls stay disabled until protocol-2 hello, configured radio, fresh
  rover telemetry, browser heartbeat, and explicit arm.
- Invalid scans stay invalid/null and never become free space.
- Pose remains unknown without track-width and distance-per-tick calibration.
- After calibration, label the view dead-reckoning estimate. Confirm an encoder
  discontinuity removes pose confidence.
- Unknown cells must never be planner-traversable. Verify obstacle footprint
  inflation using measured chassis width.

## Exit

The integrated prototype is physically accepted only after every relevant row
above and in `docs/TEST_RESULTS.md` has an operator, date, measurement, and
repeatable PASS. It remains a non-certified laboratory prototype.
