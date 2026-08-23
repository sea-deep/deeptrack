# Four-Wheel Mine Rescue Rover — Cirkit Designer Guide & Complete Netlist

## 1. Cirkit Designer Compatibility Analysis
* **Import Format Reality**: Cirkit Designer (https://app.cirkitdesigner.com) is an online, cloud-managed electronics IDE. Projects are saved directly to user cloud accounts (identified by project UUIDs).
* **Supported Import Assets**:
  - Component models: Fritzing Part files (`.fzpz`), `.svg`, and `.png` for custom parts.
  - Firmware: Arduino C++ (`sketch.ino` / `main.cpp`).
* **Direct Project Import**: Cirkit Designer does **not** support importing external CAD whole-project files (e.g., Wokwi `diagram.json`, KiCad `.kicad_sch`, or Fritzing `.fzz` project archives) via a file-upload button.
* **Recommended Workflow**: Use Cirkit Designer's built-in component library and its native **AI Autowiring Assistant** with the structured circuit prompt below, or follow the deterministic pin-to-pin netlist.

---

## 2. Bill of Materials (BOM) for Cirkit Designer Library

| # | Component | Cirkit Designer Search Name | Quantity | Operating Voltage |
|---|---|---|---|---|
| 1 | Microcontroller | `ESP32 DevKit V1` (30-pin or 38-pin) | 1 | 5V (VIN) / 3.3V (Logic) |
| 2 | Motor Driver | `L298N Motor Driver Module` or `TB6612FNG` | 1 | 5V - 12V (VM) / 5V-3.3V (VCC) |
| 3 | DC Motors | `TT Gear Motor / DC Motor` | 4 | 5V - 6V |
| 4 | Temp & Humidity | `DHT22 / AM2302 Sensor` | 1 | 3.3V - 5V |
| 5 | Gas Sensor | `MQ-2 / MQ-4 Gas Sensor Module` | 1 | 5V (Analog 0-5V) |
| 6 | Ultrasonic Sensor | `HC-SR04 Ultrasonic Distance Sensor` | 1 | 5V (Echo 5V) |
| 7 | IMU Sensor | `MPU-6050 6-Axis Gyro & Accel` | 1 | 3.3V (I2C) |
| 8 | Wheel Encoders | `KY-040 Rotary Encoder Module` | 2 | 3.3V |
| 9 | Scanning Servo | `SG90 Micro Servo 9g` | 1 | 5V (PWM) |
| 10 | Warning Buzzer | `Piezo Buzzer (Active/Passive)` | 1 | 3.3V - 5V |
| 11 | Power Bank | `10000mAh USB Power Bank` (represented by 5V DC Source / Screw Terminal + SPST Switch) | 1 | 5.0V @ 3.0A |
| 12 | Power LED | `Green 5mm LED` | 1 | 2.0V - 3.0V |
| 13 | Resistors | `220Ω` (LEDs/Buzzer), `10kΩ` & `15kΩ` (Dividers & Pull-ups) | 7 | 1/4W |

---

## 3. Comprehensive Netlist & Pin-to-Pin Wiring Table

### A. Power Distribution System (10,000 mAh USB Power Bank)
* **Power Source**: 10,000 mAh Power Bank 5V / 3A USB Output.
* **Main Switch**: SPST Switch between Power Bank `5V (+)` and Main `5V Rail`.
* **Power Indicator**: `5V Rail` -> `220Ω Resistor` -> `Green LED Anode (+)` -> `LED Cathode (-)` -> `Common GND`.
* **5V High-Current Rail**: Feeds ESP32 `VIN` (or 5V pin), Motor Driver `VM / VCC`, `SG90 Servo V+`, `HC-SR04 VCC`, `MQ Gas VCC`.
* **3.3V Logic Rail**: Provided by ESP32 `3V3` pin (internal AMS1117 LDO regulator). Feeds `DHT22 VCC`, `MPU6050 VCC`, `KY-040 Encoders VCC`.
* **Common Ground**: All GND pins (ESP32 GND, Power Bank GND, Motor Driver GND, Sensor GNDs) tied together.

### B. Logic & GPIO Pin Connections

| Subsystem | Component Pin | ESP32 GPIO / Rail | Notes & Safety Components |
|---|---|---|---|
| **DHT22** | VCC | `3V3` | Safe 3.3V rail |
| | GND | `GND` | Common Ground |
| | DATA / SDA | `GPIO 23` | 10kΩ pull-up resistor to 3V3 |
| **MQ Gas Sensor** | VCC | `5V Rail` | Requires 5V for internal heater coil |
| | GND | `GND` | Common Ground |
| | AO (Analog Out) | `GPIO 34` (ADC1_CH6) | **10kΩ / 15kΩ Voltage Divider** (scales 5V AO down to safe 3.0V max for ESP32) |
| **HC-SR04** | VCC | `5V Rail` | 5V power for ultrasonic transducer |
| | GND | `GND` | Common Ground |
| | TRIG | `GPIO 19` | 3.3V trigger signal from ESP32 |
| | ECHO | `GPIO 18` | **10kΩ / 15kΩ Voltage Divider** (scales 5V Echo down to 3.0V safe level) |
| **MPU6050** | VCC | `3V3` | 3.3V Logic Rail |
| | GND | `GND` | Common Ground |
| | SDA | `GPIO 21` | ESP32 I2C Data |
| | SCL | `GPIO 22` | ESP32 I2C Clock |
| **Left Encoder** | VCC / GND | `3V3` / `GND` | |
| | CLK | `GPIO 32` | Left wheel pulse interrupt |
| **Right Encoder**| VCC / GND | `3V3` / `GND` | |
| | CLK | `GPIO 35` | Right wheel pulse interrupt (10kΩ pull-up to 3V3) |
| **Scanning Servo**| V+ / GND | `5V Rail` / `GND`| 5V power supply |
| | PWM (Signal) | `GPIO 13` | 50Hz PWM control signal |
| **Warning Buzzer**| (+) Pin | `GPIO 4` | Connected through 220Ω resistor |
| | (-) Pin | `GND` | Common Ground |
| **4WD Motors** | Left Side (FL & RL) | `GPIO 25` | Left Motor Enable / PWM Channel |
| | Right Side (FR & RR) | `GPIO 14` | Right Motor Enable / PWM Channel |
| | Motor Driver GND | `GND` | Common Ground |

---

## 4. Cirkit Designer AI Autowiring Prompt

You can copy and paste this exact prompt into the **Cirkit Designer AI Assistant** (in the right-hand panel of Cirkit Designer) to automatically place and wire the rover circuit:

```text
Build an autonomous ESP32 4WD mine rescue rover circuit with the following parts and wiring:
1. Microcontroller: ESP32 DevKit V1.
2. Power: 10,000 mAh USB Power Bank (5V / 3A) with a power switch and a green LED power indicator with a 220-ohm resistor. Connect 5V to ESP32 VIN, and connect common GND to all components.
3. Sensors:
   - DHT22 sensor: VCC to 3.3V, GND to GND, DATA to ESP32 GPIO 23 with a 10k pull-up resistor to 3.3V.
   - MQ Gas sensor: VCC to 5V, GND to GND, AO connected to ESP32 GPIO 34 through a 10k/15k voltage divider.
   - HC-SR04 Ultrasonic sensor: VCC to 5V, GND to GND, TRIG to GPIO 19, ECHO to GPIO 18 through a 10k/15k voltage divider.
   - MPU6050 IMU: VCC to 3.3V, GND to GND, SDA to GPIO 21, SCL to GPIO 22.
   - KY-040 Left Encoder: VCC to 3.3V, GND to GND, CLK to GPIO 32.
   - KY-040 Right Encoder: VCC to 3.3V, GND to GND, CLK to GPIO 35 with 10k pull-up.
4. Actuators:
   - SG90 Micro Servo: V+ to 5V, GND to GND, PWM signal to GPIO 13.
   - Buzzer: Positive through 220-ohm resistor to GPIO 4, Negative to GND.
   - Motor Driver (L298N / TB6612): Power from 5V rail and GND. Left motor control to GPIO 25, Right motor control to GPIO 14, driving 4 DC gear motors.
Ensure all grounds are connected together and all 5V sensor outputs to ESP32 GPIOs use voltage dividers for 3.3V logic protection.
```
