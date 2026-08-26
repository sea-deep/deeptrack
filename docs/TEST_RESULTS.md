# DEEPTRACK test results

This file is the evidence ledger for the project. A physical acceptance test is never marked `PASS` from code inspection, compilation, or simulated dashboard behavior.

## Phase 0 — baseline and claim-control verification

Date: 2026-08-26
Workspace: `/home/dipak/code/deeptrack`

| Check | Command or method | Result | Evidence / notes |
|---|---|---|---|
| Rover diagnostics firmware compiles | `arduino-cli compile --fqbn esp32:esp32:esp32 rover/RoverDiagnostics` | PASS | ESP32 core 3.3.11; 389,843 bytes flash (29%), 26,024 bytes globals (7%). Compilation only; no hardware behavior inferred. |
| TB6612 motor test compiles | `arduino-cli compile --fqbn esp32:esp32:esp32 rover/TB6612Test` | PASS | ESP32 core 3.3.11; 274,416 bytes flash (20%), 22,124 bytes globals (6%). Compilation only. |
| Gateway firmware compiles | `arduino-cli compile --fqbn esp32:esp32:esp32 gateway` | PASS WITH WARNING | ESP32 core 3.3.11; 295,692 bytes flash (22%), 23,516 bytes globals (7%). `LiquidCrystal I2C` declares AVR compatibility only, so physical ESP32 LCD behavior remains unverified. |
| Dashboard static analysis | `npm run check` | PASS | `svelte-check` reported 0 errors and 0 warnings. |
| Dashboard logic tests | `npm run test:console` | PASS | 19 tests passed; 0 failed. Includes demo connection isolation and real-mode unknown-state boundaries. |
| Dashboard UI/a11y audit | `npm run audit:ui` | PASS | 19 Svelte templates scanned; 0 findings. |
| Dashboard production build | `npm run build` | PASS | SvelteKit/Vite production bundle completed. `adapter-auto` correctly defers the target adapter until a hosting platform is chosen. |
| Demo/real browser isolation | In-app browser walkthrough | PASS | Demo: simulated badge and fixtures visible, 0 Connect buttons, 0 Disconnect buttons. Real: `REAL · UNKNOWN`, one real-only Connect USB button, unknown observations/pose, empty maps/logs, no demo controls or fixtures. Browser console: 0 errors. |
| Claims and pin-map review | Repository search plus source review | PASS | Positive production/online, calibrated gas, depth, and autonomous-mapping claims removed. Remaining appearances are explicit prohibitions/safety boundaries. Firmware and dashboard pin projections match `docs/WIRING_GUIDE.md`. |

## Phase 2 — production rover safety firmware

Date: 2026-08-26

| Check | Command or method | Result | Evidence / notes |
|---|---|---|---|
| Front-safety host tests | `./scripts/firmware/test-host.sh` | PASS | Initial UNKNOWN fail-closed, forward/single-side rejection, reverse/pivot allowance, stop/clear hysteresis, invalid-but-fresh behavior, stale fail-closed, and `millis()` wrap were asserted. |
| Production rover compile | `./scripts/firmware/compile.sh rover-mission` | PASS | ESP32 core 3.3.11; 998,235 bytes flash (76%), 49,476 bytes globals (15%). Compilation only. |
| Deploy-script shell validation | `bash -n scripts/firmware/*.sh` | PASS | Explicit target and explicit serial-port selection; no hardware upload was attempted. |
| Preserved firmware regression compile | `./scripts/firmware/compile.sh rover-diagnostics`, `motor-test`, `gateway-diagnostics` | PASS WITH WARNING | Rover diagnostics and motor test compile unchanged in behavior. Gateway compiles; the existing `LiquidCrystal I2C` AVR-architecture warning remains. |
| Dashboard Phase 0 regression | `npm run check && npm run test:console && npm run audit:ui && npm run build` | PASS | 0 Svelte diagnostics, 19/19 logic tests, 0 UI/a11y findings, production build completed. |
| Phase 2 physical bench acceptance | `docs/PHASE_2_BENCH_CHECKLIST.md` | NOT RUN | Requires rover, multimeter, wheels-up direction checks, measured targets, and active-brake observation. |

## Integrated software acceptance — rover to dashboard

Date: 2026-08-26

| Check | Command or method | Result | Evidence / notes |
|---|---|---|---|
| All firmware targets compile | `./scripts/firmware/compile.sh all`, followed by final changed-target compiles | PASS WITH LCD WARNING | Rover mission: 995,419 bytes flash (75%), 49,436 bytes globals (15%). Rover diagnostics: 389,843/26,024. Motor test: 274,416/22,124. Gateway mission: 926,320 bytes flash (70%), 47,088 bytes globals (14%). Gateway diagnostics: 295,692/23,516. The LiquidCrystal I2C AVR declaration warning remains; no LCD behavior inferred. |
| Shared runtime safety | `./scripts/firmware/test-host.sh` | PASS | UNKNOWN/stale front fail-closed, hysteresis, forward/single-side gate, pivot/reverse allowance, signed encoder direction, per-side stall timing, and millis wrap tested. |
| Rover mission review | Compile plus source/threshold audit | PASS (SOFTWARE) | Default disarmed; active-brake latch; 80 ms bounded front schedule; encrypted peer config fail-closed; session/sequence/local TTL; signed one-channel odometry; independent left/right stall stop; hysteretic persistent water stop; scheduled IMU/DHT/gas; stationary valid-neighbor opening scan; no autonomous reverse. |
| Gateway mission review | Compile plus protocol audit | PASS (SOFTWARE) | Encrypted unicast peer; bounded callback copies; 450 ms browser watchdog; fresh rover required to arm; fresh-to-stale rover link forces disarm and prevents automatic resume; 125 ms command refresh; application ACK/event forwarding; USB NDJSON only. |
| Hardware-plan reconciliation | Playbook power pages plus repository BOM/wiring audit | PASS (DOCUMENTATION) | Removed obsolete 5 V power-bank-to-buck and gateway-buck instructions. BOM now matches protected 2S-to-5 V logic, separate 4xAA NiMH motor VM, five LED resistors, optional LCD level shifting, and transistor-driven two-pin buzzer requirements. No electrical behavior inferred. |
| Dashboard static/build | `npm run check && npm run audit:ui && npm run build` | PASS | 0 Svelte diagnostics, 0 UI/a11y findings, production build complete. |
| Dashboard logic | `npm run test:console` | PASS | 28 tests: 19 console/mode tests, 5 NDJSON contract tests, 4 calibrated dead-reckoning/occupancy tests. Unknown cells are not planner-traversable. |
| Browser mode regression | In-app local browser walkthrough | PASS | Demo: simulated badge/fixtures, zero Connect/Disconnect/Arm controls. Real: one Connect USB, REAL UNKNOWN, null sensor/pose/map, drive and auto disabled, no demo fixture. Console warnings/errors: 0. |
| Shell/deploy validation | `bash -n scripts/firmware/*.sh` and ignored-config check | PASS | Explicit targets/ports; radio provisioner validates MAC/channel, generates independent keys, writes ignored mode-0600 config; provisioner was not run because measured MACs are not yet recorded. |
| Integrated physical acceptance | `docs/HARDWARE_ACCEPTANCE_CHECKLIST.md` | NOT RUN | Requires assembled rover, measured rails/dividers/geometry/thresholds, controlled obstacles, link-loss tests, and recorded operators/results. |

## Connected gateway and dashboard-flow verification

Date: 2026-08-26

| Check | Command or method | Result | Evidence / notes |
|---|---|---|---|
| Gateway mission compile | `./scripts/firmware/compile.sh gateway-mission` | PASS WITH LCD WARNING | 926,132 bytes flash (70%), 47,088 bytes globals (14%). The unsupported AVR-only LCD driver is skipped at runtime so it cannot block the USB bridge. |
| Gateway upload | `./scripts/firmware/upload.sh gateway-mission /dev/ttyACM0` | PASS | ESP32-D0WD-V3 revision 3.1; flash hashes verified; base MAC `88:57:21:8e:c3:68`. |
| Gateway USB handshake | Reset pulse plus 115200-baud UART capture | PASS | Repeated protocol-v1 `hello` records received with board `gateway`, STA MAC `88:57:21:8E:C3:68`, positive session ID, `lcd_ready:false`, and `armed:false`. |
| ESP-NOW identity mapping | USB descriptors plus `esptool read-mac` | PASS | Gateway: USB serial `5AB5004467`, `/dev/ttyACM0`, STA MAC `88:57:21:8E:C3:68`. Rover: USB serial `5AB5005013`, `/dev/ttyACM1`, STA MAC `88:57:21:B6:87:3C`. Roles were verified before either paired image was uploaded. |
| Encrypted ESP-NOW initialization | Provisioner, paired uploads, gateway UART capture | PASS | Ignored mode-0600 local configuration generated on fixed channel 1; both flash hashes verified. Gateway reports `radio_ready:true`. |
| Rover → gateway telemetry | Six-second gateway UART capture | PASS | Repeated live telemetry at approximately 8 Hz with real front, gas, water, climate, IMU, encoder and safety fields; RSSI approximately -63 to -73 dBm; observed `packet_gaps:0`. |
| Gateway → rover command/ACK path | Unarmed gateway STOP refresh and rover application ACKs | PASS | Monotonic session-scoped command sequence received repeated status-0 rover ACKs. Motors remained powered off and rover drive state remained `STOPPED`. |
| Reset recovery | Unarmed rover/gateway reset capture with motor power off | PASS | Radio reinitialized, session was re-established, rover boot events arrived, and telemetry/ACK traffic resumed without motion. |
| Armed stale-link safety | Fresh telemetry gate, zero-speed arm, continued browser heartbeats, then rover reset; motor power off | PASS | Gateway first rejected a deliberately premature arm. After fresh telemetry it armed at zero speed, detected the missing rover link, emitted `GATEWAY_STOP` with `Rover link expired; explicit re-arm required`, and returned `armed:false`. Rover rebooted and telemetry recovered without automatic re-arm. A separate physical RF-range/removal test remains. |
| Public dashboard flow | In-app browser at local Vite build | PASS | Homepage opens `/dashboard`; chooser offers public `/dashboard/demo` and sign-in for the real gateway; demo contains no Connect USB control; unauthenticated `/dashboard/real` redirects to `/auth`. Browser console errors: 0. |
| Dashboard regression after route changes | `npm run check && npm run test:console && npm run build` | PASS | 0 Svelte errors/warnings; 29/29 tests pass; production bundle completes. |

## Physical acceptance matrix

These tests require the actual rover, controlled test conditions, and recorded observations. They remain deliberately unclaimed until executed.

| ID | Acceptance criterion | Status | Recorded evidence |
|---|---|---|---|
| P1-01 | Physical kill switch removes motor power while logic remains observable | NOT RUN | — |
| P1-02 | TB6612 standby/fail-safe behavior verified on the assembled rover | NOT RUN | — |
| P1-03 | Each motor direction matches the documented command convention | NOT RUN | — |
| P1-04 | Motors stop on command timeout, link loss, and controller reset | NOT RUN | — |
| P2-01 | Sensor identities and wiring match the authoritative pin map | NOT RUN | — |
| P2-02 | HC-SR04 divider voltage is safe at the ESP32 input | NOT RUN | — |
| P2-03 | I2C devices are stable under motor noise | NOT RUN | — |
| P2-04 | MQ-4 warm-up and clean-air baseline are recorded | NOT RUN | — |
| P2-05 | Water-probe dry/wet/contact thresholds are measured | NOT RUN | — |
| P2-06 | IMU offsets and axis conventions are measured | NOT RUN | — |
| P3-01 | Rover-to-gateway protocol passes packet-loss and stale-link tests | NOT RUN | — |
| P3-02 | ACK/retry behavior is verified for every safety-relevant command | NOT RUN | — |
| P4-01 | Manual driving, obstacle response, scan validity, and stale-data UI pass together | NOT RUN | — |

## Failure log

| Date | Test ID | Observation | Corrective action | Retest |
|---|---|---|---|---|
| 2026-08-26 | P3-01 | Initial encrypted ESP-NOW test was blocked because the rover USB cable did not enumerate. | Reconnected a working data cable, fixed roles by unique USB serial and measured MAC, provisioned both peers, verified live bidirectional traffic, and passed an armed stale-link reset test. Physical RF-range/removal testing remains. | PARTIAL PASS |
