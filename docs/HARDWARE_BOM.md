# DEEPTRACK authoritative hardware BOM

This list follows the 45-page implementation playbook and the locked wiring in
`docs/WIRING_GUIDE.md`. It replaces the obsolete power-bank/5 V buck mixture.
Record exact purchased module variants before assembly; clone board pinouts and
electrical limits are not interchangeable.

DEEPTRACK is a non-certified laboratory prototype. None of these parts makes it
intrinsically safe or suitable for a mine, explosive atmosphere, or rescue
deployment.

## Rover electronics and mechanics

| Item | Required specification | Qty | Acceptance note |
|---|---|---:|---|
| ESP32 development board | ESP32-WROOM-32, 30-pin, board exposes GPIO16/17 and GPIO34/35 | 1 | Confirm exact board and STA MAC; do not substitute a PSRAM board without a pin audit |
| Four-wheel chassis | Four TT/BO motors, four wheels, rigid two-layer frame | 1 | Mark the HC-SR04-facing end as physical front |
| TB6612FNG breakout | Genuine dual-H-bridge breakout with VCC, VM, STBY and both channels exposed | 2 | One board per chassis side; each motor keeps its own electrical channel while each side shares commands |
| DHT22 | Three-pin module preferred | 1 | Bare four-pin device needs an external 10 kOhm data pull-up |
| MQ-4 module | Analog output exposed; 5 V heater supply | 1 | GPIO36 input requires the documented 10 kOhm/15 kOhm divider; qualitative trend only |
| HC-SR04 | Standard 5 V ultrasonic module | 1 | ECHO requires the documented 10 kOhm/15 kOhm divider |
| MPU6050/GY-521 | I2C address 0x68 | 1 | Power from the verified 3.3 V I2C domain |
| VL53L0X breakout | I2C address 0x29, 3.3 V-compatible breakout | 1 | Mount on servo; validate `RangeStatus`, not distance alone |
| Slotted encoder modules and discs | LM393-style, usable from 3.3 V | 2 | One per driven side; GPIO34/35 need verified external pull-ups |
| Water-contact PCB | Analog output, operable from 3.3 V | 1 | Contact/relative wetness only, never depth |
| SG90-class servo | 5 V supply, 3.3 V-compatible signal | 1 | Measure current and physical 35-145 degree clearance |
| Buzzer | Three-pin active module accepting 3.3 V control, or two-pin 5 V type with transistor driver | 1 | Never drive an unverified 5 V two-pin buzzer directly from GPIO4 |
| Red and green LEDs | Standard indicator LEDs | 2 | One dedicated 220 Ohm series resistor each |

## Rover power - keep the positive rails separate

| Item | Required specification | Qty | Acceptance note |
|---|---|---:|---|
| Matched 18650 cells | Same model, capacity, age and verified condition | 2 | Series logic pack only; do not improvise with unmatched cells |
| Insulated 2-cell holder | Secure contacts and protected wiring | 1 | No loose cells on a breadboard |
| 2S BMS | Correct 2S topology and adequate measured current rating | 1 | Verify B-, midpoint, B+, and protected P+/P- for the exact board |
| 8.4 V CC/CV charger | Intended for a protected 2S lithium pack | 1 | A TP4056 is not a complete 2S charger |
| 5 V step-down converter | Genuine buck, about 3 A capability | 1 | Preset to 5.0 V from the 6.4-8.4 V protected pack before connecting loads |
| Four-cell AA holder | Secure switched holder | 1 | Supplies only both TB6612 VM inputs |
| AA NiMH cells | Same type/state, rechargeable | 4 | About 4.8 V nominal; record loaded voltage |
| NiMH charger | Correct for the selected cells | 1 | Do not charge cells in an improvised series circuit |
| Motor-power switch | Rated for motor current, physically reachable | 1 | Required de-energization path because TB6612 STBY is hardwired high |
| Fuse/protection hardware | Sized from measured load and wiring | 1 set | Place per the selected battery/BMS/switch design |

`LOGIC_5V` comes from the 2S pack through the buck. `MOTOR_POS` comes from the
four-cell NiMH holder. Join their grounds at a deliberate low-resistance common
reference; never join the two positive rails. A stable protected 5 V USB power
bank may replace the entire 2S/BMS/buck logic path, but must feed the appropriate
5 V path directly - never through a 5 V-to-5 V ordinary buck.

## Gateway

| Item | Required specification | Qty | Acceptance note |
|---|---|---:|---|
| ESP32 development board | Same audited ESP32-WROOM-32 family | 1 | Powered and connected to the laptop through one data-capable USB cable |
| 16x2 LCD with I2C backpack | PCF8574, measured address 0x27 or 0x3F | 1 | Try verified 3.3 V operation first; if 5 V is required, add bidirectional I2C level shifting |
| Bidirectional I2C level shifter | 3.3 V/5 V compatible | 1 optional | Required if the LCD backpack pulls SDA/SCL to 5 V |
| Red, yellow and green LEDs | Standard indicator LEDs | 3 | One dedicated 220 Ohm series resistor each |
| USB cable | Data-capable, matches board and laptop connectors | 1 | USB supplies the gateway; no 5 V-to-5 V buck is needed |

## Passives, protection and assembly

| Item | Minimum qty | Assignment |
|---|---:|---|
| 10 kOhm, 1/4 W resistors | 4 | Two divider upper legs plus left/right encoder pull-ups; add one for a bare DHT22 |
| 15 kOhm, 1/4 W resistors | 2 | MQ-4 and HC-SR04 divider lower legs |
| 220 Ohm, 1/4 W resistors | 5 | Two rover LEDs and three gateway LEDs |
| 1 kOhm resistor + NPN transistor | 1 each, conditional | Driver for a two-pin 5 V buzzer |
| Flyback diode | 1 conditional | Use if the selected buzzer is inductive/electromagnetic |
| 470 uF electrolytic capacitors | 2 | One near logic/servo 5 V and one near the motor VM distribution |
| 100 nF ceramic capacitors | 3 | ESP32 3.3 V and both TB6612 VCC rails |
| Multimeter | 1 | Mandatory for rail, divider, continuity and loaded-sag acceptance |
| Insulated wire, terminals, headers, heat-shrink, strain relief | As measured | Do not route motor current through weak breadboard contacts |
| Breadboards/protoboard and mounting hardware | As required | Secure modules, batteries and cables before motor tests |

## Procurement and assembly hold points

- Do not energize the rover until exact BMS terminals, buck polarity/output,
  divider values, common ground, and separate positive rails are verified.
- Do not connect external VIN while USB is attached unless the exact DevKit's
  power-path isolation has been verified.
- Do not connect a 5 V LCD backpack directly to ESP32 SDA/SCL if it pulls those
  lines above 3.3 V.
- Do not mark a substitute part accepted until its pinout, voltage domain,
  current requirement and firmware compatibility are recorded.
