# DEEPTRACK calibration record

Do not enter estimated, copied, or manufacturer-typical values in measured fields. If a measurement has not been performed, write `NOT MEASURED`.

## Run metadata

| Field | Recorded value |
| --- | --- |
| Date/time and timezone | NOT MEASURED |
| Operator | NOT MEASURED |
| Reviewer | NOT MEASURED |
| Rover firmware commit | NOT MEASURED |
| Gateway firmware commit | NOT MEASURED |
| Test surface | NOT MEASURED |
| Rover logic supply | NOT MEASURED |
| Motor battery / state | NOT MEASURED |
| Wheels lifted for direction tests | NOT MEASURED |
| Physical motor-power switch reachable | NOT MEASURED |

## Fixed radio identities

These identities were measured on 2026-08-26. Match the unique USB serial
before every upload; `/dev/ttyACM0` and `/dev/ttyACM1` names may change after
reconnection.

| Role | USB serial | Measured STA MAC | ESP-NOW channel |
| --- | --- | --- | --- |
| Gateway | `5AB5004467` | `88:57:21:8E:C3:68` | 1 |
| Rover | `5AB5005013` | `88:57:21:B6:87:3C` | 1 |

PMK and LMK values remain only in the ignored mode-0600 local configuration;
do not copy them into this record or commit them.

## Electrical baseline

Measure with the rover stationary and motors disabled first.

| Measurement | Expected basis | Recorded value | Instrument | Result |
| --- | --- | --- | --- | --- |
| Logic rail at ESP32 VIN | Regulated 5 V rail | NOT MEASURED |  | NOT RUN |
| ESP32 3V3 rail | ESP32 regulator output | NOT MEASURED |  | NOT RUN |
| Left TB6612 VCC | Shared 3V3 logic rail | NOT MEASURED |  | NOT RUN |
| Right TB6612 VCC | Shared 3V3 logic rail | NOT MEASURED |  | NOT RUN |
| Both TB6612 STBY | Hardwired to 3V3 | NOT MEASURED |  | NOT RUN |
| Left TB6612 VM | Motor battery | NOT MEASURED |  | NOT RUN |
| Right TB6612 VM | Motor battery | NOT MEASURED |  | NOT RUN |
| HC-SR04 ECHO divider maximum | Must remain ESP32-safe | NOT MEASURED |  | NOT RUN |
| MQ-4 divider maximum | Must remain ESP32-safe | NOT MEASURED |  | NOT RUN |
| Common-ground continuity | Logic, sensor, and motor grounds share reference | NOT MEASURED |  | NOT RUN |

Record brownout observations separately for idle, servo movement, one motor bank, and both motor banks. Do not continue if a rail is outside the component limit or the ESP32 resets.

## Physical-front and motor-direction calibration

The physical front is the sensor-facing end. Perform the first tests with wheels lifted and one motor bank at a time.

| Check | Command | Observed physical motion | Inversion setting | Result |
| --- | --- | --- | --- | --- |
| Left bank positive | Short bounded pulse | NOT MEASURED | NOT MEASURED | NOT RUN |
| Left bank negative | Short bounded pulse | NOT MEASURED | NOT MEASURED | NOT RUN |
| Right bank positive | Short bounded pulse | NOT MEASURED | NOT MEASURED | NOT RUN |
| Right bank negative | Short bounded pulse | NOT MEASURED | NOT MEASURED | NOT RUN |
| Both banks forward | Short bounded pulse | NOT MEASURED | NOT MEASURED | NOT RUN |
| Both banks stop | Stop command and physical switch | NOT MEASURED | — | NOT RUN |

## Chassis geometry and encoders

| Constant | Procedure | Recorded value | Repeat spread | Result |
| --- | --- | --- | --- | --- |
| Wheel diameter under load | Measure each driven wheel, record range | NOT MEASURED | NOT MEASURED | NOT RUN |
| Effective wheel circumference | Multi-revolution tape test | NOT MEASURED | NOT MEASURED | NOT RUN |
| Left ticks per revolution | At least three counted revolutions | NOT MEASURED | NOT MEASURED | NOT RUN |
| Right ticks per revolution | At least three counted revolutions | NOT MEASURED | NOT MEASURED | NOT RUN |
| Track width, contact-center to contact-center | Loaded chassis measurement | NOT MEASURED | NOT MEASURED | NOT RUN |
| Chassis width | Widest footprint plus protrusions | NOT MEASURED | NOT MEASURED | NOT RUN |
| Chassis length | Front sensor face to rear-most point | NOT MEASURED | NOT MEASURED | NOT RUN |
| One-metre estimated travel error | Repeated tape test | NOT MEASURED | NOT MEASURED | NOT RUN |

Single-channel encoders do not measure direction. Firmware signs each pulse from
the last applied logical motor direction. Record the commanded direction and
reduce confidence after slip, stalls, lifting, or contradictory IMU
observations. Stall timing is evaluated independently for the left and right
banks; event value `1` means left, `2` right, and `3` both.

## Front distance sensors and scanner

Phase 2 currently uses conservative implementation starting points, not measured acceptance results:

| Firmware constant | Current starting value | Validation status |
| --- | --- | --- |
| HC-SR04 bounded echo wait | 12,000 µs | UNVALIDATED ON HARDWARE |
| Accepted HC-SR04 range | 2-200 cm | UNVALIDATED ON HARDWARE |
| Forward stop boundary | 24 cm | UNVALIDATED ON HARDWARE |
| Forward clear boundary | 32 cm | UNVALIDATED ON HARDWARE |
| Last-valid freshness window | 300 ms | UNVALIDATED ON HARDWARE |
| Manual motor duty | 145 / 255 | UNVALIDATED ON HARDWARE |
| Manual burst duration | 600 ms | UNVALIDATED ON HARDWARE |
| Motor ramp | 12 duty every 30 ms | UNVALIDATED ON HARDWARE |
| Encoder stall window | 650 ms above 100 duty | UNVALIDATED ON HARDWARE |
| Water contact hysteresis | contact at 60% of dry-to-wet span for 3 samples; clear at 40% for 5 samples | UNVALIDATED ON HARDWARE |
| Auto/turn duty | 125 / 120 | UNVALIDATED ON HARDWARE |
| Scan angles | 35, 55, 75, 90, 105, 125, 145° | UNVALIDATED ON HARDWARE |
| Servo settle | 110 ms | UNVALIDATED ON HARDWARE |
| Accepted ToF scan range | 30-1200 mm with `RangeStatus == 0` | UNVALIDATED ON HARDWARE |
| Tilt caution / persistent stop / clear | 12° / 25° for 500 ms / 20° | UNVALIDATED ON HARDWARE |
| DHT interval | 2500 ms | UNVALIDATED ON HARDWARE |
| Gas filter | median-of-5, EMA alpha 0.20 | UNVALIDATED ON HARDWARE |
| Gateway heartbeat timeout | 450 ms | SOFTWARE VERIFIED ONLY |
| Rover command TTL | 300 ms nominal, 500 ms hard maximum | SOFTWARE VERIFIED ONLY |
| Left/right inversion defaults | `true` / `true` | UNCONFIRMED PHYSICAL DIRECTION |

Change a starting value only after recording the relevant bench result below.

| Check | Procedure | Recorded value | Result |
| --- | --- | --- | --- |
| HC-SR04 offset | Compare several tape-measured targets | NOT MEASURED | NOT RUN |
| HC-SR04 invalid/stale behavior | Disconnect or obstruct sensor safely | NOT MEASURED | NOT RUN |
| VL53L0X center offset | Compare several matte targets | NOT MEASURED | NOT RUN |
| VL53L0X invalid range status | Exercise out-of-range/poor target | NOT MEASURED | NOT RUN |
| Servo physical center | Align to physical front | NOT MEASURED | NOT RUN |
| Servo safe minimum angle | Verify cable and chassis clearance | NOT MEASURED | NOT RUN |
| Servo safe maximum angle | Verify cable and chassis clearance | NOT MEASURED | NOT RUN |
| Scanner left/right sign | Observe known target on each side | NOT MEASURED | NOT RUN |
| Measured stopping distance | Loaded rover, recorded speed/surface/battery | NOT MEASURED | NOT RUN |

## IMU

| Check | Procedure | Recorded value | Result |
| --- | --- | --- | --- |
| Stationary gyro Z bias | Average while motionless | NOT MEASURED | NOT RUN |
| Level pitch/roll offset | Record on verified level surface | NOT MEASURED | NOT RUN |
| Left/right turn sign | Compare against physical direction | NOT MEASURED | NOT RUN |
| Quarter-turn estimate | Repeat measured 90-degree turns | NOT MEASURED | NOT RUN |
| Tilt advisory point | Bench test only; justify chosen value | NOT MEASURED | NOT RUN |

The MPU6050 provides no absolute yaw reference. Heading remains relative and confidence-bearing.

## Environmental sensors

| Check | Procedure | Recorded value | Result |
| --- | --- | --- | --- |
| DHT22 sample cadence | Confirm at least two seconds between reads | NOT MEASURED | NOT RUN |
| DHT22 placement offset | Compare against a reference away from heat sources | NOT MEASURED | NOT RUN |
| MQ-4 initial conditioning | Record accumulated powered duration | NOT MEASURED | NOT RUN |
| MQ-4 session warm-up | Record duration and stabilization evidence | NOT MEASURED | NOT RUN |
| MQ-4 clean-air raw baseline | Median and spread in safe ambient air | NOT MEASURED | NOT RUN |
| MQ-4 qualitative trend threshold | Justify from safe, non-flammable test evidence | NOT MEASURED | NOT RUN |
| Water dry baseline | Record clean/dry raw value and spread | NOT MEASURED | NOT RUN |
| Water contact response | Use safe clean water, then clean/dry probe | NOT MEASURED | NOT RUN |

Never use a flame, accumulated lighter gas, fuel, or an unknown atmosphere to test the MQ-4. This prototype cannot declare air safe.

## Commands that persist measured values

Run these only after performing and recording the corresponding measurement.
They write ESP32 Preferences; `cal clear` erases them and locks auto/mapping
again.

```text
cal motors <left-inverted 0|1> <right-inverted 0|1>
cal servo <measured-center 80..100> <low-angles-point left|right>
cal width <widest chassis millimetres>
cal track <driven-wheel contact-center spacing millimetres>
cal tickum <measured micrometres travelled per encoder tick>
cal turn90 <measured low-duty 90-degree pivot milliseconds>
cal waterdry
cal waterwet
cal gasbase
cal imulevel
cal gyrobias
cal clear
```

`cal gasbase` records only a qualitative raw-signal baseline. It is not ppm,
%LEL, certification, or evidence that an atmosphere is safe. Auto mode requires
measured motor/servo direction, servo center, chassis width, and turn timing.
Estimated pose additionally requires
track width and distance per encoder tick. Water contact classification requires
both dry and wet points separated by at least 100 ADC counts.
Run `cal imulevel` only on a verified level surface. Run `cal gyrobias` only
while the rover is completely stationary; firmware samples for three seconds
and rejects a noisy or implausibly large bias.

## Sign-off

| Role | Name | Date | Decision |
| --- | --- | --- | --- |
| Operator |  |  | NOT REVIEWED |
| Electrical reviewer |  |  | NOT REVIEWED |
| Firmware reviewer |  |  | NOT REVIEWED |
