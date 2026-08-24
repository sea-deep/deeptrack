# DEEPTRACK — Complete Hardware Shopping List

Everything you need to buy for both the Rover and the Gateway.
All audit fixes are baked in (separate 5V bus regulated by a buck converter, MQ-4 voltage divider, decoupling caps). Both ESP32 boards use **USB Type-C**, not Micro-USB.

> Search on **Robu.in**, **Amazon.in**, **Quartz Components**, or your local electronics shop.
> Prices are approximate mid-2026 INR.

---

## ROVER (Goes Inside the Mine)

### Brain + Chassis + Power

| # | What to Search | Spec | Qty | ₹ Est. | Why |
|---|---|---|---|---|---|
| 1 | ESP32 DevKit V1 (30-pin) | ESP32-WROOM-32, **USB Type-C** | 1 | 350–500 | Rover MCU. 30-pin fits breadboard with 1 row free on each side |
| 2 | 4WD Robot Car Chassis Kit | Acrylic 2-layer, includes 4× TT motors + 4× 65mm wheels + screws + standoffs | 1 kit | 400–650 | If your kit does NOT include motors/wheels, buy 4× "BO Motor 200RPM" (₹40–60 each) and 4× "65mm Robot Wheel" (₹20–30 each) separately |
| 3 | 10000mAh Power Bank | 5V output, 2A or higher. USB-A output port | 1 | 500–900 | Must sustain 2A+ continuous draw. Avoid ultra-slim (auto-shutoff on low current). Mi/Ambrane/Syska work well |
| 4 | USB-A to Type-C Cable (short) | 30cm data cable | 1 | 60–100 | Power bank → ESP32 (rover's ESP32 has a Type-C port, not Micro-USB). Keep it short so it doesn't snag on the chassis |
| 5 | USB-A Breakout Board | Female USB-A socket on a small PCB with screw terminals (5V, GND) | 1 | 30–50 | Cut a spare USB cable OR use this breakout to tap 5V directly from the power bank for the separate 5V bus. Feeds into the buck converter below. See Wiring Guide for details |
| 5a | **Buck Converter Module** (LM2596 or MP1584EN) | Adjustable DC-DC step-down, screw terminals or header pins, input up to 24–40V, output adjustable via trimpot | 1 | 60–120 | Regulates the tap from the power bank/battery down to a clean, stable **5V** for the separate 5V bus (motors, servo, MQ-4, HC-SR04). Trim the output to 5.0V with a multimeter **before** connecting any sensor. Sits between the USB-A Breakout Board and the 5V bus |

### Motor Driver

| # | What to Search | Spec | Qty | ₹ Est. | Why |
|---|---|---|---|---|---|
| 6 | TB6612FNG Dual Motor Driver Module | Breakout board (NOT bare chip). Pins: VM, VCC, GND, STBY, AIN1, AIN2, PWMA, AO1, AO2, BIN1, BIN2, PWMB, BO1, BO2 | 2 | 400–700 | One H-bridge channel per motor. Two modules provide 4 independent output channels, avoiding two TT motors sharing one channel |

### Sensors

| # | What to Search | Spec | Qty | ₹ Est. | Why |
|---|---|---|---|---|---|
| 7 | DHT22 Sensor Module (3-pin) | AM2302 on a small PCB, 3 pins: VCC, DATA, GND | 1 | 180–280 | Temperature + humidity. The 3-pin module has an onboard 10kΩ pull-up. If you get the bare 4-pin sensor instead, you must add your own 10kΩ pull-up to 3V3 |
| 8 | MQ-4 Gas Sensor Module | Blue PCB, 4 pins: VCC, GND, AO, DO | 1 | 150–220 | Methane (CH4) detection. **Must be powered from 5V** (heater coil). AO output is 0–5V, needs voltage divider to GPIO (included in resistors below) |
| 9 | HC-SR04 Ultrasonic Sensor | 4-pin: VCC, TRIG, ECHO, GND | 1 | 50–90 | Forward obstacle distance. ECHO is 5V logic — needs voltage divider (included in resistors below) |
| 10 | MPU6050 GY-521 Module | 6-axis accel + gyro, I2C, address 0x68 | 1 | 120–200 | Tilt / orientation sensing. Onboard 3.3V regulator + I2C pull-ups |
| 11 | VL53L0X GY-VL53L0XV2 Module | Time-of-Flight laser distance, I2C, address 0x29 | 1 | 250–400 | Mounts on the servo for sweeping obstacle scan. Onboard regulator + pull-ups |
| 12 | LM393 Speed Sensor Module | Slotted optical sensor + comparator PCB + encoder disc | 2 | 40–70 each | One per side. Disc attaches to motor shaft, sensor counts slots. DO = digital pulse output |
| 13 | Water Level Sensor Module | Flat PCB with exposed traces, 3 pins: VCC, GND, SIG | 1 | 30–60 | Flood detection. Power from 3.3V so output stays within ESP32 ADC range |

### Actuators + Indicators

| # | What to Search | Spec | Qty | ₹ Est. | Why |
|---|---|---|---|---|---|
| 14 | SG90 Micro Servo 9g | 180° rotation, 3 wires (brown=GND, red=5V, orange=signal) | 1 | 80–130 | Mounts the VL53L0X for scanning sweep (30°–150°). Powered from 5V bus |
| 15 | Active Piezo Buzzer | 5V, 2-pin | 1 | 15–30 | Danger alarm. Code uses `tone()` so a passive buzzer also works |
| 16 | 5mm Red LED | Standard through-hole | 1 | 5–10 | Danger indicator |
| 17 | 5mm Green LED | Standard through-hole | 1 | 5–10 | Normal status |

### Resistors + Capacitors

| # | What to Search | Value | Qty | ₹ Est. | Purpose |
|---|---|---|---|---|---|
| 18 | 10kΩ Resistor ¼W | 10kΩ | 4 | 2–5 each | 1× DHT22 pull-up (skip if 3-pin module), 1× HC-SR04 ECHO divider upper, 1× MQ-4 AO divider upper, 1× right encoder pull-up |
| 19 | 15kΩ Resistor ¼W | 15kΩ | 2 | 2–5 each | 1× HC-SR04 ECHO divider lower, 1× MQ-4 AO divider lower |
| 20 | 220Ω Resistor ¼W | 220Ω | 3 | 2–5 each | 1× buzzer, 1× red LED, 1× green LED |
| 21 | 470µF Electrolytic Capacitor | 470µF 16V (or 25V) | 2 | 5–10 | One near each TB6612 VM/GND input to absorb motor startup/current spikes |
| 22 | 100nF Ceramic Capacitor | 0.1µF (code "104") | 3 | 2–5 each | 1× across ESP32 3V3/GND, 1× across each TB6612 VCC/GND |

> **Tip:** Buy a resistor assortment kit (₹80–120) and a capacitor assortment kit (₹60–100) instead of individual pieces. You'll have spares for mistakes.

---

## GATEWAY (Sits at Base Desk, Connected to Laptop)

| # | What to Search | Spec | Qty | ₹ Est. | Why |
|---|---|---|---|---|---|
| 23 | ESP32 DevKit V1 (30-pin) | Same as Rover — **USB Type-C** | 1 | 350–500 | Receives ESP-NOW from Rover, shows on LCD, sends to laptop via USB Serial |
| 24 | 16×2 I2C LCD Module | LCD1602 with PCF8574 I2C backpack soldered on. 4 pins: VCC, GND, SDA, SCL. Address 0x27 | 1 | 150–220 | Displays live telemetry. Blue backlight / white text. Has a small blue contrast potentiometer on the back — adjust with screwdriver until text is visible |
| 25 | 5mm Red LED | Standard | 1 | 5–10 | Danger |
| 26 | 5mm Green LED | Standard | 1 | 5–10 | Normal / Link OK |
| 27 | 5mm Yellow LED | Standard | 1 | 5–10 | Heartbeat — blinks on each received packet |
| 28 | 220Ω Resistor ¼W | 220Ω | 3 | 2–5 each | 1 per LED |
| 29 | **USB-C to USB-C Cable** (1–1.5m) | Data-capable cable, 1–1.5m | 1 | 100–180 | Gateway ESP32 → Laptop. **No USB-A cable is used here** — the Gateway ESP32 has a Type-C port, so this needs a laptop with a USB-C port. If your laptop only has USB-A ports, get a USB-C-to-USB-A cable instead (see note below) |
| 29a | **Buck Converter Module** (LM2596 or MP1584EN) | Adjustable DC-DC step-down, screw terminals or header pins | 1 | 60–120 | Regulates the Gateway's 5V rail (LCD + LEDs + ESP32) to a clean, trimmed 5.0V — same role as the Rover's buck converter, just on the Gateway side. Sits between the incoming 5V source and the Gateway's 5V bus |

---

## WIRING + ASSEMBLY SUPPLIES

| # | What to Search | Qty | ₹ Est. | Why |
|---|---|---|---|---|
| 30 | Half-size Breadboard (400 pts) | 2 | 80–120 each | 1 for Rover (mounts on chassis), 1 for Gateway |
| 31 | Male-to-Male Jumper Wires 20cm | 1 pack (40 wires) | 80–120 | Breadboard-to-breadboard |
| 32 | Male-to-Female Jumper Wires 20cm | 1 pack (40 wires) | 80–120 | ESP32 headers → sensor modules |
| 33 | Female-to-Female Jumper Wires 20cm | 1 pack (20 wires) | 60–80 | Module-to-module when both have male headers |
| 34 | Double-sided foam tape | 1 roll | 20–30 | Mount breadboard + sensors on chassis |
| 35 | Small zip ties | 1 pack | 20–30 | Cable management on chassis |
| 36 | Small Phillips screwdriver | 1 | 20–40 | Chassis assembly + LCD contrast adjustment |

---

## DASHBOARD CONNECTION (How the Cable Works)

> **Cable note:** Both ESP32 boards in this list use **USB Type-C**, not Micro-USB. If your laptop has a USB-A port and no USB-A cable is on hand, buy a **USB-C to USB-A** cable instead of Type-C to Type-C for item #29 — either works as the data + power link, just match it to the port your laptop actually has.

The Gateway ESP32 stays plugged into your laptop via USB cable (#29). This single cable:
1. **Powers the Gateway** — 5V from laptop USB
2. **Carries serial data** — Gateway prints JSON at 115200 baud

On the laptop:
1. Open `DASHBOARD/dashboard.html` in **Google Chrome** (or Edge / Chromium)
2. Click **"Connect Serial"**
3. Select the Gateway's port (`/dev/ttyUSB0` on Linux, `COMx` on Windows)
4. Dashboard goes live — gauges, path trace, and event log update in real-time

> **Web Serial requires Chrome/Edge/Chromium.** Firefox and Safari do NOT support it.

> **Linux permission fix** if you get "Permission denied":
> ```
> sudo usermod -a -G dialout $USER
> ```
> Log out and back in.

---

## COST SUMMARY

| Section | ₹ Estimate |
|---|---|
| Rover (ESP32 + Chassis + Sensors + Driver + Actuators + Passives + Caps + Buck Converter) | 2,470 – 3,990 |
| Gateway (ESP32 + LCD + LEDs + Resistors + Cable + Buck Converter) | 760–1,270 |
| Wiring Supplies (Breadboards + Wires + Tape) | 350–550 |
| **TOTAL** | **3,580 – 5,810** |

> If you already own a 10000mAh power bank, subtract ₹500–900.

---

## PRINTABLE CHECKLIST

```text
ROVER:
[ ] ESP32 DevKit V1 ×1
[ ] 4WD Chassis Kit (4× TT motors, 4× wheels, acrylic frame) ×1
[ ] TB6612FNG Motor Driver Module ×2
[ ] DHT22 Sensor Module (3-pin preferred) ×1
[ ] MQ-4 Gas Sensor Module ×1
[ ] HC-SR04 Ultrasonic Sensor ×1
[ ] MPU6050 GY-521 Module ×1
[ ] VL53L0X GY-VL53L0XV2 Module ×1
[ ] LM393 Speed Sensor + Encoder Disc ×2
[ ] Water Level Sensor Module ×1
[ ] SG90 Micro Servo ×1
[ ] Active Piezo Buzzer ×1
[ ] 5mm Red LED ×1
[ ] 5mm Green LED ×1
[ ] 10kΩ Resistors ×4
[ ] 15kΩ Resistors ×2
[ ] 220Ω Resistors ×3
[ ] 470µF Electrolytic Capacitor ×2
[ ] 100nF Ceramic Capacitors ×3
[ ] 10000mAh Power Bank ×1
[ ] Short USB-A to Type-C Cable (30cm) ×1
[ ] USB-A Breakout Board ×1
[ ] Buck Converter Module (LM2596/MP1584EN) ×1

GATEWAY:
[ ] ESP32 DevKit V1 (USB Type-C) ×1
[ ] 16×2 I2C LCD (PCF8574) ×1
[ ] 5mm Red LED ×1
[ ] 5mm Green LED ×1
[ ] 5mm Yellow LED ×1
[ ] 220Ω Resistors ×3
[ ] USB-C to USB-C (or USB-C to USB-A) Cable (1–1.5m) ×1
[ ] Buck Converter Module (LM2596/MP1584EN) ×1

WIRING:
[ ] Breadboards ×2
[ ] M-M Jumper Wires (40pc) ×1
[ ] M-F Jumper Wires (40pc) ×1
[ ] F-F Jumper Wires (20pc) ×1
[ ] Double-sided foam tape ×1
[ ] Zip ties ×1
[ ] Small screwdriver ×1
```
