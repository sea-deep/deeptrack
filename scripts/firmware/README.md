# Firmware commands

The scripts use explicit target names so the production rover image cannot be confused with a diagnostic sketch.

```bash
# Discover the real serial device first.
arduino-cli board list

# Compile every known target without touching hardware.
./scripts/firmware/compile.sh all

# Run the host-side Phase 2 gate tests.
./scripts/firmware/test-host.sh

# After reading each board's measured STA MAC from its serial/hello output,
# generate the ignored encrypted peer configuration.
./scripts/firmware/provision-radio.sh ROVER_STA_MAC GATEWAY_STA_MAC CHANNEL

# Upload only the named image to the named device.
./scripts/firmware/upload.sh rover-mission /dev/ttyUSB0

# Open the 115200-baud serial console.
./scripts/firmware/monitor.sh /dev/ttyUSB0
```

Available targets:

- `rover-mission`: integrated production rover sensing, safety, radio, telemetry, and stationary scan/turn/recheck autonomy (`rover-phase2` is an alias).
- `rover-diagnostics`: existing comprehensive bench diagnostic.
- `motor-test`: existing basic TB6612 wiring diagnostic.
- `gateway-mission`: production gateway ESP-NOW/watchdog/USB-NDJSON firmware.
- `gateway-diagnostics`: existing LCD/LED diagnostic.

The upload script never guesses a port. It accepts only an explicit `/dev/ttyUSB*` or `/dev/ttyACM*` path, recompiles the selected target, and then uploads that build directory.

Both mission images fail closed when `DeeptrackRadioConfig.local.h` is absent,
contains a different local MAC, or contains zero keys. The provisioning script
creates that ignored file with mode `0600` and independent random 16-byte
PMK/LMK values. It does not upload either board.
