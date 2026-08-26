# DEEPTRACK

DEEPTRACK is a four-wheel, dual-ESP32 laboratory rover prototype for offline reconnaissance experiments. The defensible project claim is:

> An offline, hazard-aware reconnaissance rover with local collision protection, explainable obstacle avoidance, estimated route mapping, and a live operator dashboard.

## Safety boundary

This is an educational, non-certified bench prototype. Do not operate it in a coal mine, explosive atmosphere, confined space with unknown air quality, or real rescue scene. Its ESP32 boards, brushed motors, exposed wiring, MQ-4 heater, servo, regulators, and batteries are not an intrinsically safe assembly.

Keep the physical motor-power switch reachable during every powered test. The dashboard stop control is a remote software request and never replaces physical power isolation.

## Current implementation

The software path now spans the playbook's rover, gateway, dashboard, and
estimated-mapping phases. Physical commissioning is still incomplete:

- the physical build is four TT motors, two TB6612 drivers, one rover ESP32, and one gateway ESP32;
- `docs/WIRING_GUIDE.md` is the electrical and pin authority;
- `firmware/shared/DeeptrackHardware.h` is the compiled firmware projection of that pin map;
- `firmware/shared/DeeptrackProtocol.h` is the compiled versioned rover/gateway packet contract;
- `firmware/shared/DeeptrackThresholds.h` is the single authority for unvalidated software starting values;
- `rover/DeeptrackRover/DeeptrackRover.ino` contains scheduled sensing, active-brake motor control, local collision protection, paired ESP-NOW commands/ACKs, telemetry, and stationary scan/turn/recheck autonomy;
- `gateway/DeeptrackGateway/DeeptrackGateway.ino` contains the paired ESP-NOW peer, browser-heartbeat watchdog, LCD/LED status, and USB NDJSON bridge;
- the real dashboard consumes only gateway NDJSON, requires a fresh rover link before arming, uses hold-to-drive, and stops/disarms on focus loss or disconnect;
- real pose and occupancy evidence stay unknown until measured encoder/chassis calibration is present; the map is dead reckoning, never SLAM;
- the existing rover and gateway sketches remain separate bench diagnostics;
- dashboard demo data is explicitly marked simulated;
- MQ-4 data remains raw/qualitative, the water probe reports contact evidence only, and mapping is described as estimated rather than SLAM.

## Authority order

When sources disagree, resolve them in this order:

1. Measured physical hardware and manufacturer limits.
2. [`docs/WIRING_GUIDE.md`](docs/WIRING_GUIDE.md).
3. [`firmware/shared/DeeptrackHardware.h`](firmware/shared/DeeptrackHardware.h).
4. The current implementation playbook and recorded calibration/test evidence.
5. Older concept documents for product intent only.

Never copy motor, STBY, DHT22, MQ-4, encoder, or ultrasonic pins from the obsolete 2WD concept.

## Responsibility split

| Layer | Owns | Must never own |
| --- | --- | --- |
| Rover ESP32 | Motor output, local front-safety gate, sensor sampling, bounded motion states, command expiry | Large maps, browser-dependent collision safety, unbounded planning |
| Gateway ESP32 | ESP-NOW validation, application heartbeat watchdog, USB NDJSON bridge, LCD/LED state | Continued motion after the laptop heartbeat expires |
| Laptop dashboard | Operator controls, explicit data provenance, estimated pose/map, logs, replay, optional planning | The only collision detector or only emergency-stop path |

## Repository

```text
firmware/shared/     Compiled hardware, safety-state, and protocol contracts
firmware/tests/      Host-side safety-state tests
rover/DeeptrackRover Production-track rover mission firmware
rover/               Preserved rover diagnostics and motor bring-up sketches
gateway/             Gateway hardware-test sketch
dashboard/           SvelteKit operator-interface prototype
docs/                Wiring, calibration, test, design, and BOM evidence
scripts/firmware/    Explicit compile, upload, monitor, and host-test commands
```

No radio keys or measured calibration values are committed. Both mission images
therefore compile but start disarmed and radio-disabled in a clean checkout.
Follow [`docs/HARDWARE_ACCEPTANCE_CHECKLIST.md`](docs/HARDWARE_ACCEPTANCE_CHECKLIST.md)
before any floor test.

## Reproducible checks

Requirements: Arduino CLI, `esp32:esp32` core 3.3.11, the libraries used by the sketches, Node.js, and the committed dashboard lockfile.

```sh
./scripts/firmware/test-host.sh
./scripts/firmware/compile.sh all

cd dashboard
npm ci
npm run check
npm run build
npm run test:console
npm run audit:ui
```

Record software and physical results in [`docs/TEST_RESULTS.md`](docs/TEST_RESULTS.md). Record every measured hardware constant in [`docs/CALIBRATION.md`](docs/CALIBRATION.md); never replace an unmeasured value with a plausible number.

## Delivery order

0. Phase 0 — truth, contracts, and evidence structure.
1. Phase 1 — exact pins, reversed-chassis inversion, short motor bursts, active brake, and component diagnostics.
2. Phase 2 — bounded HC-SR04 sampling, validity/freshness, hysteresis, and a non-bypassable local forward gate.
3. Phase 3 — stationary servo/ToF scan, validated openings, low-speed turn, and fresh front recheck without blind reverse.
4. Phase 4 — paired ESP-NOW, packet/session validation, acknowledgements, command TTL, gateway heartbeat watchdog, and USB NDJSON.
5. Phase 5 — bounded-rate environmental/IMU/encoder observations with raw/qualitative provenance and freshness.
6. Phase 6 — honest live dashboard transport, hold-to-drive, STOP on blur/disconnect, deterministic logs, and explicit state labels.
7. Phase 7 — estimated pose, sparse occupancy evidence, footprint inflation, branch memory, and optional risk-aware A* on known free space.

Software checks for all layers are repeatable. Physical phase exits remain open
until their measured rows are recorded in `docs/TEST_RESULTS.md`.
