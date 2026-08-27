# DEEPTRACK

[![Deploy on Railway](https://railway.com/button.svg)](https://deeptrack.up.railway.app)
[![Demo Video](https://img.shields.io/badge/YouTube-Demo%20Video-red?logo=youtube)](https://youtube.com)

**Live Dashboard:** [https://deeptrack.up.railway.app](https://deeptrack.up.railway.app)

DEEPTRACK is a four-wheel, dual-ESP32 laboratory rover prototype built for offline reconnaissance experiments.

> An offline, hazard-aware reconnaissance rover with local collision protection, explainable obstacle avoidance, estimated route mapping, and a live operator dashboard.

---

## Safety boundary

This is an educational, non-certified bench prototype. Do not operate it in a coal mine, explosive atmosphere, confined space with unknown air quality, or real rescue scene. Its ESP32 boards, brushed motors, exposed wiring, MQ-4 heater, servo, regulators, and batteries are not an intrinsically safe assembly.

Keep the physical motor-power switch reachable during every powered test. The dashboard stop control is a remote software request and never replaces physical power isolation.

---

## What the system does

The rover carries sensors and drives under operator or autonomous control. A separate gateway ESP32 plugs into a laptop over USB and bridges the radio link to a browser dashboard. There is no internet connection on the rover side; everything is local.

| Layer | Hardware | Role |
| --- | --- | --- |
| Rover | ESP32-WROOM-32 | Drives 4x TT motors via 2x TB6612FNG drivers, reads 8 sensors, runs local collision protection and command expiry |
| Gateway | ESP32-WROOM-32 | Bridges ESP-NOW radio packets to USB NDJSON, runs a hardware watchdog, drives an LCD and 3 status LEDs |
| Dashboard | Laptop browser | SvelteKit app that consumes the USB NDJSON stream, shows estimated pose and map, sends hold-to-drive commands |

### Sensors on the rover

| Sensor | GPIO | What it measures |
| --- | --- | --- |
| HC-SR04 | 19 (TRIG) / 18 (ECHO via 10k/15k divider) | Forward clearance, ultrasonic |
| VL53L0X | I2C 0x29, SDA 21 / SCL 22 | Forward clearance, time-of-flight, mounted on servo |
| SG90 servo | 13 | Rotates VL53L0X for stationary scans |
| MPU6050 | I2C 0x68, SDA 21 / SCL 22 | 6-DOF IMU: pitch, roll, heading |
| DHT22 | 23 | Temperature and humidity |
| MQ-4 | 36 (VP) via 10k/15k divider | Gas activity — raw ADC only, not calibrated ppm |
| Water probe | 39 (VN) | Water contact — raw ADC only |
| LM393 encoders | 34 (left) / 35 (right) | Wheel ticks for dead-reckoning odometry |

The fused forward gate uses the nearer valid reading from HC-SR04 and the centered VL53L0X. If either channel is invalid or stale, forward motion is blocked regardless of what the laptop requests.

### Radio protocol

Both ESP32 boards use `firmware/shared/DeeptrackProtocol.h`. The link is encrypted unicast ESP-NOW. Packets carry a magic value, version, session ID, monotonic sequence, message type, and sender uptime. The current protocol is **version 2**. Rover, gateway, and dashboard must all run version 2; a version mismatch causes the rover to latch active brake and print `PROTOCOL_MISMATCH ... SAFE_STOP`.

Normal drive command TTL is 300 ms. The gateway watchdog expects a browser heartbeat within 450 ms. If either expires, the rover brakes and disarms. A new gateway session always starts disarmed; the operator must explicitly arm before any motion command is accepted.

### Dashboard

The dashboard has two modes:

- **Demo** — runs entirely in the browser with explicitly labelled simulated data. No hardware required. Labelled `DEMO · SIMULATED` in the header.
- **Hardware** — connects to the gateway over Web Serial. The operator must arm before driving. The interface stops and disarms automatically on tab blur, page hide, disconnect, or heartbeat expiry.

The dashboard shows six telemetry tiles (MQ-4, climate, water, front clearance, distance from start, Sentinel AI advisory), a hold-to-drive D-pad with keyboard shortcuts (W A S D, Space = stop), an estimated top-down map with configurable ToF scan ray, obstacle, and frontier overlays, mission mode controls (Manual, Explore, Navigate, Return Home), a safety-state panel listing each sensor's live status, and a tilt horizon display for pitch/roll/heading. All sensor data carries explicit provenance labels (SIMULATED, LIVE, UNKNOWN, STALE).

---

## Authority order

When sources disagree, resolve them in this order:

1. Measured physical hardware and manufacturer limits.
2. [`docs/WIRING_GUIDE.md`](docs/WIRING_GUIDE.md)
3. [`firmware/shared/DeeptrackHardware.h`](firmware/shared/DeeptrackHardware.h)
4. The current implementation playbook and recorded calibration/test evidence.
5. Older concept documents — product intent only.

Never copy motor, STBY, DHT22, MQ-4, encoder, or ultrasonic pins from any obsolete 2WD concept sketch.

---

## Responsibility split

| Layer | Owns | Must never own |
| --- | --- | --- |
| Rover ESP32 | Motor output, local front-safety gate, sensor sampling, bounded motion states, command expiry | Large maps, browser-dependent collision safety, unbounded planning |
| Gateway ESP32 | ESP-NOW validation, application heartbeat watchdog, USB NDJSON bridge, LCD/LED state | Continued motion after the laptop heartbeat expires |
| Laptop dashboard | Operator controls, explicit data provenance, estimated pose and map, logs, replay, optional planning | The only collision detector or only emergency-stop path |

---

## Current commissioning status

Software for all phases is implemented and passes host-side checks. Physical commissioning is paused pending assembled-rover tests.

| Area | Status |
| --- | --- |
| Manual drive, active brake, front gate | Implemented; physical stop-distance test pending |
| HC-SR04 + VL53L0X fused gate | Implemented; threshold calibration on physical rover pending |
| ESP-NOW v2 packet contract | Implemented; over-air validation pending |
| Encoder acquisition (raw, accepted, rejected counters, intervals) | Stationary isolation passed; full odometry calibration pending |
| Estimated pose and occupancy map (5 cm cells, ray casting) | Implemented; requires measured ticks-per-metre and track width |
| A\*, frontier Explore, Navigate, Return Home | Implemented; automatically locked until pose is calibrated |
| Dashboard NDJSON bridge | Implemented and tested against the committed lockfile |
| Supabase offline sync | Implemented; cloud sync is not in the rover safety or command path |

MQ-4 data is raw ADC only. The water probe reports contact evidence only. The map is dead reckoning, not SLAM. Demo data is explicitly labelled simulated and never stored as real hardware evidence.

---

## Reproducible checks

Requirements: Arduino CLI, `esp32:esp32` core 3.3.11, the Arduino libraries referenced in the sketches, Node.js, and the committed dashboard lockfile.

```sh
# Firmware host tests and compile
./scripts/firmware/test-host.sh
./scripts/firmware/compile.sh all

# Dashboard
cd dashboard
npm ci
npm run check
npm run build
npm run test:console
npm run audit:ui
```

No radio keys or measured calibration values are committed. Both mission images compile but start disarmed and radio-disabled in a clean checkout.

Follow [`docs/HARDWARE_ACCEPTANCE_CHECKLIST.md`](docs/HARDWARE_ACCEPTANCE_CHECKLIST.md) before any floor test. Mission motion remains calibration-gated until you pass [`docs/PHYSICAL_ODOMETRY_VALIDATION.md`](docs/PHYSICAL_ODOMETRY_VALIDATION.md).

Record software and physical test results in [`docs/TEST_RESULTS.md`](docs/TEST_RESULTS.md). Record every measured hardware constant in [`docs/CALIBRATION.md`](docs/CALIBRATION.md); never replace an unmeasured value with a plausible number.

---

## Delivery phases

0. Phase 0 — truth, contracts, and evidence structure.
1. Phase 1 — exact pins, reversed-chassis inversion, short motor bursts, active brake, component diagnostics.
2. Phase 2 — bounded HC-SR04 sampling, validity/freshness, hysteresis, non-bypassable local forward gate.
3. Phase 3 — stationary servo/ToF scan, validated openings, low-speed turn, fresh front recheck.
4. Phase 4 — paired ESP-NOW, packet/session validation, acknowledgements, command TTL, gateway heartbeat watchdog, USB NDJSON.
5. Phase 5 — bounded-rate environmental/IMU/encoder observations with raw/qualitative provenance and freshness labels.
6. Phase 6 — live dashboard transport, hold-to-drive, STOP on blur/disconnect, deterministic logs, explicit state labels.
7. Phase 7 — estimated pose, sparse occupancy evidence, footprint inflation, branch memory, A\* on known free space.

Software checks for all layers are repeatable. Physical phase exits remain open until their measured rows are recorded in `docs/TEST_RESULTS.md`.

---

## Repository structure

```
deeptrack/
├── firmware/
│   ├── shared/                     Shared C++ headers used by both ESP32 images
│   │   ├── DeeptrackHardware.h     GPIO pin authority for rover and gateway boards
│   │   ├── DeeptrackProtocol.h     Versioned ESP-NOW packet structs (currently v2)
│   │   ├── DeeptrackThresholds.h   Software starting values for distances, timing, PWM
│   │   ├── DeeptrackFrontSafety.h  Front gate logic shared by rover and tests
│   │   ├── DeeptrackRuntimeSafety.h Safety-state machine
│   │   └── DeeptrackRadioConfig.h  MAC and channel configuration placeholder
│   └── tests/                      Host-side unit tests for safety-state logic
│
├── rover/
│   ├── DeeptrackRover/
│   │   └── DeeptrackRover.ino      Production rover firmware — motors, sensors,
│   │                               encoders, ESP-NOW, scan autonomy
│   ├── RoverDiagnostics/           Bench diagnostic sketch, kept for component bring-up
│   └── TB6612Test/                 Motor driver bring-up sketch
│
├── gateway/
│   ├── DeeptrackGateway/
│   │   └── DeeptrackGateway.ino    Production gateway firmware — ESP-NOW peer,
│   │                               USB NDJSON bridge, LCD/LED, watchdog
│   └── gateway.ino                 Legacy hardware test sketch
│
├── dashboard/                      SvelteKit operator interface
│   ├── src/
│   │   ├── routes/
│   │   │   ├── +page.svelte        Landing page with mode selection
│   │   │   ├── auth/               Supabase auth flow
│   │   │   └── dashboard/
│   │   │       └── +page.svelte    Main dashboard — console, maps, logs, hardware views
│   │   └── lib/
│   │       ├── components/         Svelte UI components (NavRail, MetricTile,
│   │       │                       TiltHorizon, AiRiskPanel, EstimatedRouteCanvas, ...)
│   │       ├── state/              Protocol parsing, map state, mission controller,
│   │       │                       manual drive logic
│   │       ├── ai/                 Hazard engine (loads hazard-model.json at runtime)
│   │       ├── config/             Rover calibration constants used by the dashboard
│   │       ├── offline/            Supabase offline-first sync layer
│   │       ├── schemas/            Zod schemas for NDJSON record validation
│   │       └── utils/              Motion helpers, theme, formatting
│   ├── scripts/                    Node.js console test suite
│   └── static/                     Static assets including hazard-model.json
│
├── docs/
│   ├── WIRING_GUIDE.md             Complete electrical and pin authority
│   ├── HARDWARE_BOM.md             Authoritative bill of materials with acceptance notes
│   ├── CALIBRATION.md              Template for recording measured hardware constants
│   ├── TEST_RESULTS.md             Record of all software and physical test outcomes
│   ├── HARDWARE_ACCEPTANCE_CHECKLIST.md  Pre-floor-test checklist
│   ├── PHYSICAL_ODOMETRY_VALIDATION.md   Commissioning sequence for odometry
│   ├── PROTOCOL.md                 ESP-NOW and USB NDJSON protocol reference
│   ├── AUTONOMY_GAP_AUDIT.md       Audit of implemented vs. physically validated features
│   ├── DESIGN.md                   Material 3 design system spec for the dashboard
│   └── ANIMATION.md                Animation and motion specification
│
├── supabase/
│   ├── config.toml                 Supabase project configuration
│   └── migrations/                 SQL migrations for dashboard offline sync tables
│
├── scripts/
│   └── firmware/                   Shell scripts for compile, upload, monitor, host tests
│
├── railway.json                    Railway deployment config (build and start commands)
└── package.json                    Root package delegating build/start to dashboard/
```

### Key files at a glance

| File | What it does |
| --- | --- |
| `firmware/shared/DeeptrackHardware.h` | Single source for every GPIO number. Contains compile-time assertions. Change here and nowhere else. |
| `firmware/shared/DeeptrackProtocol.h` | Binary packet structs exchanged over ESP-NOW. Both boards must match the version field. |
| `rover/DeeptrackRover/DeeptrackRover.ino` | Complete rover mission firmware. Manages all motion states, safety gates, sensor reads, encoder capture, and radio. |
| `gateway/DeeptrackGateway/DeeptrackGateway.ino` | Receives ESP-NOW packets, validates them, converts to USB NDJSON, and watches for heartbeat from the laptop. |
| `dashboard/src/routes/dashboard/+page.svelte` | The operator UI. Protocol parsing, map estimation, mission control, and all display logic in one SvelteKit page. |
| `docs/WIRING_GUIDE.md` | Electrical authority. Pin aliases, voltage rails, divider values, BMS wiring, and common-ground rules. |
| `docs/CALIBRATION.md` | The only place measured hardware constants live. Nothing goes into firmware until recorded here first. |
