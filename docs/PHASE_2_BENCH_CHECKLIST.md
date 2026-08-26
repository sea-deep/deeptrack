# Phase 2 bench checklist

> Historical gate for the HC-SR04 safety foundation. The current mission image
> now includes later integrated phases; use
> `docs/HARDWARE_ACCEPTANCE_CHECKLIST.md` for present commissioning. The
> `rover-phase2` target remains only a compatibility alias to `rover-mission`,
> so the old boot text and per-side commands below are not the current mission
> interface. Use the preserved `rover-diagnostics` sketch for per-component
> direction checks.

Scope: production-track motor control plus bounded HC-SR04 front safety. This phase does not claim autonomous avoidance, rear protection, ESP-NOW, gateway control, or dashboard-to-rover control.

Stop at the first failed step. Keep the rover's physical motor-power switch reachable. Raise all wheels before the first motion command.

## 1. Build and identify the board

```bash
./scripts/firmware/test-host.sh
./scripts/firmware/compile.sh rover-phase2
arduino-cli board list
```

Do not guess the serial device. Record the exact `/dev/ttyUSB*` or `/dev/ttyACM*` path.

## 2. Electrical preconditions — power off first

| ID | Check | Pass condition | Result |
|---|---|---|---|
| EL-01 | Logic rail | Buck output remains close to the measured 5.0 V target under the planned load | NOT RUN |
| EL-02 | Common ground | Motor negative and ESP32 ground have continuity; positive rails remain separate | NOT RUN |
| EL-03 | Input dividers | A controlled 5.0 V input produces about 3.0 V at the MQ-4 and HC-SR04 divider junctions, never above 3.3 V | NOT RUN |
| EL-04 | Driver standby | Both TB6612 STBY pins measure approximately 3.3 V | NOT RUN |
| EL-05 | Startup motion | Reset/power-up leaves all wheels stopped | NOT RUN |

Do not upload or power motors if EL-01 through EL-04 fail.

## 3. Upload the production-track image

```bash
./scripts/firmware/upload.sh rover-phase2 /dev/ttyUSB0
./scripts/firmware/monitor.sh /dev/ttyUSB0
```

Replace `/dev/ttyUSB0` with the discovered device. Expected boot structure:

```text
=== DEEPTRACK ROVER MISSION FIRMWARE / PHASE 2 ===
PWM left=OK right=OK
FIRMWARE=DEEPTRACK_ROVER PHASE=2 MODE=MANUAL_ONLY
FRONT sample=<VALID|INVALID> freshness=<FRESH|NEVER> gate=<CLEAR|BLOCKED> ...
```

The measured distance is intentionally not predicted here.

## 4. Front validity, freshness, and hysteresis

Use broad, flat targets at measured distances. Type `front` after each placement.

| ID | Action | Expected serial/state | Result |
|---|---|---|---|
| SEN-02A | Target beyond 32 cm but within 200 cm | `sample=VALID freshness=FRESH gate=CLEAR` with a plausible measured distance | NOT RUN |
| SEN-02B | Move target into 24-32 cm band after CLEAR | Gate remains CLEAR; hysteresis prevents chatter | NOT RUN |
| SEN-02C | Target at or below 24 cm | Gate changes to BLOCKED | NOT RUN |
| SEN-02D | Move target into 24-32 cm band after BLOCKED | Gate remains BLOCKED | NOT RUN |
| SEN-02E | Target at or beyond 32 cm | Gate changes to CLEAR | NOT RUN |
| SEN-03 | Remove/block ECHO until the last valid sample is older than 300 ms | `freshness=STALE gate=BLOCKED`; a forward request is rejected | NOT RUN |

No return is never displayed as zero distance or infinite clearance.

## 5. Direction and active-brake tests — wheels lifted

With a valid clear front target, use these 600 ms commands one at a time:

```text
leftfwd
leftrev
rightfwd
rightrev
fwd
rev
left
right
brake
```

| ID | Pass condition | Result |
|---|---|---|
| DRV-01 | `leftfwd/leftrev` move only the two left wheels toward/away from the ultrasonic-facing physical front | NOT RUN |
| DRV-02 | `rightfwd/rightrev` move only the two right wheels in the correct directions | NOT RUN |
| DRV-03 | `fwd` moves all wheels toward the physical front; default inversion flags are confirmed or corrected in source | NOT RUN |
| DRV-04 | A moving wheel stops repeatably when `brake` is sent and when the 600 ms burst expires | NOT RUN |
| SEN-03B | With front state BLOCKED/STALE, `leftfwd`, `rightfwd`, and `fwd` all print `REJECTED` and do not move | NOT RUN |

`coast` exists only to compare the explicit high-impedance state during a controlled wheels-up test. It is not the normal stop command.

## 6. Phase 2 exit gate

Phase 2 is physically complete only when every EL, DRV, and SEN row above has a recorded result in `docs/TEST_RESULTS.md`, including actual stop observations. Compilation and host tests alone do not satisfy this gate.
