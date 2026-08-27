# Physical odometry validation milestone

Date prepared: 2026-08-27
Scope: protocol-v2 commissioning, motor/encoder isolation, loaded odometry,
and manual-mode safety only. This runbook does not enable autonomous motion.

## Current checkpoint — physical work paused

Physical commissioning paused on 2026-08-27 after the following bounded
evidence was collected:

- paired protocol-v2 boot, live telemetry/STOP acknowledgements, and the
  protocol-v1 `SAFE_STOP` probe passed;
- three stationary 10-second observations recorded zero raw, accepted,
  debounce-rejected, and state-rejected edges on both encoders;
- with motor power disabled, rotating the left disc produced 541 left raw
  edges and zero right raw edges; rotating the right disc produced 95 right raw
  edges and zero left raw edges in the bounded snapshot;
- three wheels-lifted `test side left forward 90 2000` runs recorded left raw
  counts of 140, 130, and 203, with inactive-right raw counts of 1, 0, and 0;
- the same runs rejected 109/140, 102/130, and 173/203 left raw edges through
  the provisional 1500 µs debounce rule (approximately 78%, 78%, and 85%).

Stationary noise and manual disc isolation therefore pass. Left-forward motor
response is only preliminary: the single inactive-right transition remains a
transient to recheck, and the high/variable rejection ratio requires signal-
quality diagnosis before distance calibration. Accepted counts from the manual
disc tests were zero because motor-state gating was active; their isolation
decision is based on raw counts.

The gateway's periodic disarmed STOP refresh cancels motor validation during
the countdown. The completed left-forward runs were made only after fully
powering off the gateway. Do not treat that workaround as a communication-loss
test or alter the radio safety behavior merely to run diagnostics.

No ticks-per-metre, track width, minimum reliable PWM, turn, or stopping value
has been accepted. Physical motor and odometry work is now deliberately paused;
do not run left reverse, either right-side motor direction, PWM sweep, straight,
turn, or ground tests until the user explicitly resumes commissioning.

## Stop conditions

Keep the physical motor-power switch reachable. Remove motor power immediately
for unexpected motion, smoke, heat, loose wiring, sensor fault, or loss of
control. Type `stop` to cancel a leased test. Motor tests use a five-second
countdown, PWM 80–170, at most 3 seconds for a side/turn and 5 seconds for a
straight run. The validation runner sends a lease every 200 ms; firmware
actively brakes if it is absent for 750 ms. Boot is disarmed and has no test
autorun. The legacy `fwd`, `rev`, `left`, and `right` serial bursts are disabled.

Do not change `encscale`, `encdebounce`, or `encgate` to make cross-talk look
better. Raw edges are the electrical/optical evidence. Leave `encgate off`
during isolation. Do not calculate a side scale until both inactive-side tests
pass.

## Identify, build, flash, and verify protocol v2

With motor power **off**, connect one ESP32 at a time and identify it by stable
USB serial and STA MAC, not by the transient `/dev/ttyACM#` suffix:

```bash
arduino-cli board list
lsusb
udevadm info --query=property --name=/dev/ttyACM0 | rg 'ID_SERIAL|ID_SERIAL_SHORT'
udevadm info --query=property --name=/dev/ttyACM1 | rg 'ID_SERIAL|ID_SERIAL_SHORT'
```

The confirmed identities are gateway USB serial `5AB5004467`, STA MAC
`88:57:21:8E:C3:68`, and rover USB serial `5AB5005013`, STA MAC
`88:57:21:B6:87:3C`. They were most recently observed as `/dev/ttyACM0` and
`/dev/ttyACM1` respectively, but reconfirm them after reconnection and never
assume the old device suffix.

Build the two exact target names derived from `scripts/firmware/targets.sh`:

```bash
./scripts/firmware/compile.sh rover-mission
./scripts/firmware/compile.sh gateway-mission
```

After substituting the newly confirmed suffixes:

```bash
./scripts/firmware/upload.sh rover-mission /dev/ttyACM_ROVER
./scripts/firmware/upload.sh gateway-mission /dev/ttyACM_GATEWAY
```

The placeholders are deliberately invalid; replace each with an actual device
such as `/dev/ttyACM1`. Open two terminals:

```bash
./scripts/firmware/monitor.sh /dev/ttyACM_ROVER
./scripts/firmware/monitor.sh /dev/ttyACM_GATEWAY
```

Pass only when the rover prints `PROTOCOL=2`, the gateway emits a hello record
containing `"board":"gateway"` and `"protocol":2`, `radio_ready` is true, live
telemetry reports protocol 2, and the rover stays disarmed. With the gateway
disarmed, type this line into its serial console:

```json
{"type":"protocol_probe","version":1}
```

Pass only when the gateway reports `PROTOCOL_PROBE_SENT`, the rover prints
`PROTOCOL_MISMATCH expected=2 received=1 SAFE_STOP`, and the brake remains
latched. Never run the mismatch probe while intentionally moving.

## Numbered physical sequence

1. **Electrical preflight.** Motor switch off. Verify common ground, logic and
   motor rails, TB6612 STBY, safe HC-SR04 divider voltage, GPIO34/35 external
   pull-ups, secure discs/sensors, and clear wheels. Pass: values match the
   wiring guide and neither motor moves at boot. Otherwise stop.

2. **Configuration baseline.** On the rover console run `config show`, then
   `encgate off`, `resetenc`, `enc`. Save the transcript. Pass: brake is latched,
   rover is disarmed, both inputs show a definite 0 or 1, and raw counters are
   visible. Confirm `front` reports a fresh valid clear path before any forward
   command; invalid or stale front data must reject forward. Do not tune
   debounce yet.

3. **Test A — stationary noise.** Run:

   ```bash
   mkdir -p logs/physical-validation
   ./scripts/firmware/run-validation.py /dev/ttyACM_ROVER \
     "test noise 10" --log logs/physical-validation/A-noise.log
   ```

   Repeat three times, including once with the servo and nearby electronics
   powered normally. Preferred result is zero raw edges. Provisional pass is
   no more than one isolated raw edge per side in 10 seconds, no repeatable
   bursts, and stable input levels. Any repeatable activity or motor-correlated
   burst fails even if accepted counts are zero.

4. **Optical/electrical hand check.** Motor power off; rotate each disc slowly
   by hand while watching `enc`. Pass: only its assigned raw counter increments,
   its digital level switches cleanly, and the other raw counter stays within
   the stationary tolerance. A swapped counter, stuck level, or cross-count is
   a failure.

5. **Test B — left only, wheels lifted.** Start at PWM 90:

   ```bash
   ./scripts/firmware/run-validation.py /dev/ttyACM_ROVER \
     "test side left forward 90 2000" --log logs/physical-validation/B-left-fwd.log
   ./scripts/firmware/run-validation.py /dev/ttyACM_ROVER \
     "test side left reverse 90 2000" --log logs/physical-validation/B-left-rev.log
   ```

   Pass: both left wheels visibly follow the named direction, left raw and
   signed accepted counts are consistent, and right raw is at most the larger
   of one edge or 0.5% of left raw **and** is explained by the measured idle
   baseline. Otherwise fail isolation; gyro data cannot excuse it.

6. **Test C — right only.** Repeat with `test side right forward 90 2000` and
   `test side right reverse 90 2000`. Apply the mirror-image criterion. If a
   motor direction is wrong, brake, determine the inversion bits, run
   `cal motors <left 0|1> <right 0|1>`, reboot, and repeat both sides. Do not
   infer direction from encoder totals alone.

7. **Test D — low/medium/cruise PWM.** After isolation passes, repeat each side
   and direction at PWM 90, 120, and the repository cruise value 145, at least
   three starts each. Example: `test side left forward 120 2000`. Pass: 3/3
   starts turn smoothly, no stall/oscillation, raw tick rate increases
   plausibly, debounce rejection is below 1%, and the smallest legitimate
   interval retains margin above `min_pulse_us`. If legitimate pulses approach
   or fall below it, stop and inspect the unfiltered pulse train before changing
   the value. Record the lowest PWM with reliable loaded starts later; a lifted
   wheel value is not sufficient.

8. **Both directions, lifted.** Run `test straight forward 90 2000`,
   `test straight reverse 90 2000`, `test turn left 90 1500`, and
   `test turn right 90 1500`. Pass: the physical motion matches the command,
   signed accepted counts have the expected signs, and no side stalls. These
   tests establish direction only, not geometry.

9. **Loaded straight calibration.** Use the actual payload and floor. Mark a
   short course that can be traversed within the 5-second bound. Perform at
   least five forward and five reverse runs at each reliable low, medium (120),
   and cruise (145) PWM. Use `test straight forward|reverse <PWM> <ms>` and
   measure actual distance from the same chassis datum; do not assume the mark
   was reached. For each run compute `abs(ticks)/measured_distance_m`. Take the
   median separately for left and right; report median absolute deviation and
   range. Pass: within-direction variation ≤3%, forward/reverse median
   disagreement ≤5%, and no missed starts or unexplained rejected/raw pulses.

10. **Loaded in-place rotation.** With the straight scales fixed, run at least
    five 90° and five 180° turns in both directions and at each tested speed,
    using a duration learned conservatively without exceeding 3000 ms. Compute
    wheel distances `dL=ticksL/leftTPM`, `dR=ticksR/rightTPM`, then effective
    track width `abs(dR-dL)/abs(angle_rad)`. Use the median across accepted
    clockwise/counter-clockwise runs. Pass after applying it when median error
    is ≤5° for 90° and ≤10° for 180°, with no direction-specific bias above
    those limits.

11. **Minimum PWM and maximum tick rate.** On loaded ground, lower PWM in small
    bounded steps but never below the firmware test floor of 80. Minimum
    reliable PWM is the lowest value that starts 5/5 times in both directions
    without stall or oscillation. Maximum plausible tick rate is the largest
    observed raw Hz at cruise plus the observed run-to-run margin; confirm its
    minimum interval is not rejected by `min_pulse_us`. Record these as physical
    evidence; do not enable balance correction. Once all isolation and loaded
    straight gates pass, record conservative gains while disarmed:

    ```text
    balance config <encoder_kp> <gyro_kp> <max_pwm_trim> <window_ms>
    balance encoder on
    balance gyro on
    config show
    config save
    ```

    Each enable command rejects missing geometry/gains; gyro balance also
    rejects a missing stationary gyro-bias calibration. Start one correction
    source at a time, repeat the straight course, and disable it immediately
    with `balance encoder off` or `balance gyro off` if variance worsens.

12. **Emergency obstacle stop and stopping distance.** In the real dashboard,
    select 40%, explicitly arm manual control, hold forward on the marked floor,
    and approach a rigid target with a soft sacrificial barrier and a spotter at
    the motor switch. Repeat five times at every accepted speed under full
    payload. Measure distance from the first ≤stop-threshold valid sample to
    zero wheel motion, preferably with timestamped telemetry and high-frame-rate
    video. Pass: every run brakes before contact; invalid/stale front readings
    also block forward; there is no automatic resume. The configured stop
    threshold must be at least the worst measured stopping distance plus sensor,
    timing, and floor margin; resume must be greater than stop.

13. **Communication-loss stop.** Wheels lifted first, then a clear-floor repeat.
    At 40% manual forward, disconnect/close the dashboard USB connection without
    releasing the drive control. Pass: motor command reaches zero and active
    brake latches within 500 ms (the rover command TTL is 300 ms; gateway
    heartbeat timeout is 450 ms), gateway disarms, and reconnect does not
    re-arm. Repeat by removing gateway power/radio. Failure disables all motion
    beyond attended bench diagnosis.

14. **Persist and reboot-verify.** Only after all isolation gates pass, apply
    medians (example placeholders must be replaced):

    ```text
    encscale left <median_left_ticks_per_metre>
    encscale right <median_right_ticks_per_metre>
    cal track <median_effective_track_width_mm>
    config safety <validated_stop_m> <validated_resume_m>
    config save
    config show
    ```

    Reboot the rover and run `config show` again. Pass: exact values reappear,
    a short straight and turn confirmation stays within the gates, and auto
    remains disabled. `config reset` resets encoder/safety commissioning values;
    `cal clear` clears all rover calibration and must only be used deliberately.

## Cross-counting decision tree

If the inactive side produces raw edges, stop scale calibration:

1. Motor power off, rotate only the active disc by hand. If inactive raw rises,
   inspect GPIO assignment, wiring swaps, interrupt registration, shared signal
   wiring, ground continuity, and external pull-ups on GPIO34/GPIO35.
2. If hand rotation isolates correctly but an unpowered motor run does not,
   inspect disc alignment, looseness/wobble, LM393 gap, comparator-pot threshold,
   digital-output stability, mechanical vibration, and ambient-light leakage.
3. If edges correlate with PWM rather than optical interruption, separate sensor
   wires from motor/PWM cables; inspect common ground, motor EMI, suppression
   capacitors, motor and sensor supply noise, and comparator decoupling. Compare
   motor powered with wheels prevented from optically crossing only under safe
   mechanical control.
4. Observe the sensor digital output and supply with a logic analyzer/scope. A
   real transition at the GPIO with matching raw increments is a physical/input
   problem. A clean inactive GPIO with raw increments indicates GPIO mapping,
   interrupt registration, memory/accounting, or wiring identity error.
5. Correct one cause, return to Test A, then repeat B and C. Do not use scale
   factors, gating, debounce, discarded pulses, or gyro correction as a pass.

## Measurement sheet

| Run | Test/direction | PWM | ms | distance m / angle ° | L raw | R raw | L accepted | R accepted | L/R debounce | L/R state | L/R interval min-avg-max µs | stop distance m | visible result |
|---:|---|---:|---:|---|---:|---:|---:|---:|---|---|---|---:|---|
| 1 |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 2 |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 3 |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 4 |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 5 |  |  |  |  |  |  |  |  |  |  |  |  |  |

Summary to calculate: median left/right ticks per metre; min/median/max and
MAD; per-side variation; forward/reverse disagreement; median effective track
width; 90°/180° error by direction; minimum reliable loaded PWM; maximum raw
tick Hz and minimum legitimate interval; worst stopping distance by PWM.

## Logs to return

Return the complete `VALIDATION_RESULT` logs for all A–D runs, pre/post-reboot
`config show`, rover and gateway boot/hello logs, the protocol-probe output,
the filled measurement sheet, actual distance/angle measurement method, payload
mass, battery voltage, floor material, wheel condition, motor-direction notes,
and timestamped/video evidence for obstacle and link-loss stops. Do not trim raw
or rejected counters. For a failure, report the exact gate, observation, likely
cause, and the next single diagnostic action.

## Waypoint gate

Waypoint following, exploration, Return Home, and encoder/gyro balance are now
implemented behind calibration and live-safety gates. They must remain
operationally disabled until every numbered acceptance gate above passes.
Mapping, planning, and odometry are software foundations only until this
physical evidence exists.
