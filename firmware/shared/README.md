# Shared firmware contracts

These headers are compiled by both ESP32 targets.

- `DeeptrackHardware.h` contains the firmware representation of the physical pin map in `docs/WIRING_GUIDE.md`.
- `DeeptrackFrontSafety.h` contains the pure Phase 2 front-validity, freshness, hysteresis, and forward-gate state machine used by the rover and host tests.
- `DeeptrackProtocol.h` locks the versioned rover/gateway wire representation and conservative timing constants before Phase 4 ESP-NOW integration.
- `DeeptrackThresholds.h` centralizes commissioning starting values. It does not make them measured or calibrated.
- `DeeptrackRadioConfig.h` fails closed unless an ignored `DeeptrackRadioConfig.local.h` has measured peer MACs and nonzero keys.

Electrical wiring, power rails, level shifting, and resistor requirements remain authoritative in `docs/WIRING_GUIDE.md`. Packet changes must be deliberate: update the protocol version, both firmware targets, the gateway NDJSON adapter, and the recorded compatibility test together.
