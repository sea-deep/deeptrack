# DEEPTRACK — Circuit Wiring Guide

Step-by-step pin-by-pin wiring for both the Rover and Gateway.
All connections are **corrected** — diagram simulation shortcuts are replaced with real-hardware-safe wiring.

---

## CRITICAL DIFFERENCE FROM WOKWI DIAGRAM

The Wokwi diagram routes all 5V loads through the ESP32's tiny 5V pin. On real hardware this **will** brownout and potentially damage the board. This guide uses a **separate 5V power bus**.

### Power Bus Setup (Do This First)

```
POWER BANK (USB-A port)
    │
    ├──[USB-A Breakout Board or cut USB cable]──→ +5V BUS (red wire)
    │                                             │
    │                                             ├→ ESP32 VIN pin
    │                                             ├→ TB6612FNG VM pin
    │                                             ├→ SG90 Servo V+ (red wire)
    │                                             ├→ HC-SR04 VCC
    │                                             ├→ MQ-4 Module VCC
    │                                             │
    │                                       [470µF cap across +5V and GND]
    │
    └──────────────────────────────────────→ GND BUS (black wire)
                                              │
                                              ├→ ESP32 GND (any GND pin)
                                              ├→ TB6612FNG GND (both GND pins)
                                              ├→ All sensor GND pins
                                              ├→ All LED cathodes (via their circuits)
                                              └→ Buzzer negative pin
```

**How to tap 5V from the power bank:**
- **Option A (cleanest):** Use a USB-A female breakout board. Plug the power bank's USB cable into it. The breakout exposes 5V and GND as screw terminals or header pins.
- **Option B (quick):** Cut a spare USB-A cable. The **red** wire is +5V, **black** is GND. Strip, tin, and connect to breadboard rails.

The ESP32 gets its 5V through the **VIN** pin (not through Micro-USB in this setup). The onboard AMS1117 regulator converts VIN → 3.3V for the ESP32's logic.

> You can ALSO power the ESP32 via its Micro-USB (plug a second cable from the power bank). In that case, skip connecting VIN and use the ESP32's `5V` pin only for the 3.3V LDO input — do NOT draw motor/servo current from it.

---

## ROVER ESP32 — Complete Pin Map

### GPIO Assignment Table

| GPIO | Direction | Connected To | Wire Color | Notes |
|---|---|---|---|---|
| **VIN** | Power In | 5V Bus (+) | Red | Powers the ESP32 via onboard regulator |
| **GND** | Power | GND Bus | Black | Use multiple GND pins to distribute current |
| **3V3** | Power Out | 3.3V rail for logic sensors | Red | Max ~600mA from LDO. Feeds: DHT22, MPU6050, VL53L0X, encoders, water sensor |
| **23** | Output | DHT22 DATA | Green | + 10kΩ pull-up to 3V3 (skip if using 3-pin module) |
| **34** | ADC Input | MQ-4 AO (via voltage divider) | Orange | Input-only pin, no pull-up available |
| **19** | Output | HC-SR04 TRIG | Cyan | 3.3V trigger pulse, HC-SR04 accepts it fine |
| **18** | Input | HC-SR04 ECHO (via voltage divider) | Yellow | 5V → 3.0V through 10k/15k divider |
| **21** | I2C SDA | MPU6050 SDA + VL53L0X SDA | Blue | Shared I2C bus, different addresses |
| **22** | I2C SCL | MPU6050 SCL + VL53L0X SCL | Purple | Shared I2C bus |
| **32** | Input (interrupt) | Left LM393 Speed Sensor DO | Green | Rising edge interrupt for pulse counting |
| **35** | Input (interrupt) | Right LM393 Speed Sensor DO | Green | Input-only pin. Needs external 10kΩ pull-up to 3V3 |
| **36 (VP)** | ADC Input | Water Level Sensor SIG | Cyan | Input-only pin. Powered from 3V3, output 0–3.3V |
| **13** | PWM Output | SG90 Servo signal (orange wire) | Purple | 50Hz PWM for servo position |
| **4** | Output | Buzzer (+) via 220Ω resistor | Orange | `tone()` generates alarm frequency |
| **26** | Output | Red LED anode via 220Ω | Red | Danger indicator |
| **27** | Output | Green LED anode via 220Ω | Green | Normal indicator |
| **25** | PWM Output | TB6612FNG PWMA | Green | Left motor speed (LEDC CH0, 5kHz) |
| **16** | Output | TB6612FNG AIN1 | Blue | Left motor direction bit 1 |
| **17** | Output | TB6612FNG AIN2 | Orange | Left motor direction bit 2 |
| **14** | PWM Output | TB6612FNG PWMB | Green | Right motor speed (LEDC CH1, 5kHz) |
| **33** | Output | TB6612FNG BIN1 | Blue | Right motor direction bit 1 |
| **2** | Output | TB6612FNG BIN2 | Orange | Right motor direction bit 2. **Caution:** GPIO2 is a boot strapping pin — disconnect this wire if flashing fails |
| **5** | Output | TB6612FNG STBY | Purple | Pull HIGH to enable driver, LOW = standby |

---

## ROVER — Step-by-Step Wiring

### Step 1: Power Rails on Breadboard

1. Run a **red wire** from the 5V bus to the breadboard's **+** rail (top)
2. Run a **black wire** from the GND bus to the breadboard's **−** rail (top)
3. Bridge the top and bottom power rails with jumper wires (red + to +, black − to −)
4. Plug the **ESP32** into the breadboard, centered, straddling the middle gap
5. Connect ESP32 **VIN** → breadboard **+5V rail**
6. Connect ESP32 **GND** (any GND pin) → breadboard **GND rail**
7. Place the **470µF electrolytic capacitor** across the +5V and GND rails. **Long leg (+) to +5V, short leg (−) to GND**

> The ESP32's `3V3` output pin now provides 3.3V for logic-level sensors.

### Step 2: DHT22 Temperature + Humidity Sensor

```
DHT22 Module (3-pin):
    VCC  →  ESP32 3V3
    GND  →  GND rail
    DATA →  ESP32 GPIO 23

If using bare 4-pin DHT22:
    Pin 1 (VCC) →  ESP32 3V3
    Pin 2 (DATA) → ESP32 GPIO 23
    Pin 3         (not connected)
    Pin 4 (GND)  → GND rail
    + 10kΩ resistor between Pin 1 (VCC/3V3) and Pin 2 (DATA)
```

### Step 3: MQ-4 Gas Sensor (with Voltage Divider)

The MQ-4 module outputs 0–5V on AO. ESP32 GPIO34 max is 3.3V. The voltage divider scales 5V down to 3.0V.

```
MQ-4 Module:
    VCC  →  5V bus (NOT 3V3!)
    GND  →  GND rail
    AO   →  [10kΩ resistor] → junction point → ESP32 GPIO 34
                                    │
                               [15kΩ resistor]
                                    │
                                  GND rail

Voltage at junction = 5V × 15k/(10k+15k) = 3.0V max ← safe
```

**Wiring on breadboard:**
1. MQ-4 AO pin → one end of 10kΩ resistor (Row A)
2. Other end of 10kΩ → junction row (Row B)
3. From junction row (Row B) → jumper wire to ESP32 GPIO34
4. From junction row (Row B) → one end of 15kΩ resistor
5. Other end of 15kΩ → GND rail

### Step 4: HC-SR04 Ultrasonic Sensor (with Voltage Divider)

```
HC-SR04:
    VCC  →  5V bus
    GND  →  GND rail
    TRIG →  ESP32 GPIO 19 (direct, 3.3V trigger is accepted by HC-SR04)
    ECHO →  [10kΩ resistor] → junction → ESP32 GPIO 18
                                   │
                              [15kΩ resistor]
                                   │
                                 GND rail

Voltage at junction = 5V × 15k/(10k+15k) = 3.0V max ← safe
```

Identical divider circuit to the MQ-4. Build it the same way on the breadboard.

### Step 5: MPU6050 IMU (I2C Bus)

```
MPU6050 GY-521:
    VCC  →  ESP32 3V3
    GND  →  GND rail
    SDA  →  ESP32 GPIO 21
    SCL  →  ESP32 GPIO 22
    (AD0, INT, XDA, XCL — leave unconnected)
```

No external pull-ups needed — the GY-521 module has 4.7kΩ pull-ups onboard.

### Step 6: VL53L0X ToF Sensor (I2C Bus — Shared with MPU6050)

```
VL53L0X GY-VL53L0XV2:
    VIN  →  ESP32 3V3
    GND  →  GND rail
    SDA  →  ESP32 GPIO 21  (same wire/row as MPU6050 SDA)
    SCL  →  ESP32 GPIO 22  (same wire/row as MPU6050 SCL)
    (XSHUT, GPIO1 — leave unconnected)
```

Both I2C devices share the same SDA/SCL lines. They have different addresses (MPU=0x68, VL53=0x29) so there's no conflict. Both modules have onboard pull-ups.

> **Mount the VL53L0X on the SG90 servo horn** using double-sided tape or a small bracket. The servo sweeps 30°–150°, and the VL53L0X scans distances at each angle.

### Step 7: LM393 Speed Sensors (Wheel Encoders)

```
Left Speed Sensor:
    VCC  →  ESP32 3V3
    GND  →  GND rail
    DO   →  ESP32 GPIO 32

Right Speed Sensor:
    VCC  →  ESP32 3V3
    GND  →  GND rail
    DO   →  ESP32 GPIO 35
    + 10kΩ pull-up resistor between ESP32 3V3 and GPIO 35
      (GPIO35 is input-only, has no internal pull-up)
```

**Mounting:** Attach the slotted encoder disc to one of the motor shafts on each side. Position the LM393 sensor so the disc's slots pass through the sensor's optical gap. Use hot glue or zip ties to secure.

### Step 8: Water Level Sensor

```
Water Level Sensor:
    VCC  →  ESP32 3V3  (NOT 5V — keeps output within 0–3.3V)
    GND  →  GND rail
    SIG  →  ESP32 GPIO 36 (VP)
```

### Step 9: SG90 Scanning Servo

```
SG90 Servo (3 wires):
    Brown wire (GND)    →  GND rail
    Red wire (V+)       →  5V bus (NOT through ESP32)
    Orange wire (Signal) →  ESP32 GPIO 13
```

> Power the servo from the 5V bus, not the ESP32's 5V pin. Servo stall current can reach 700mA.

### Step 10: Buzzer

```
Buzzer:
    (+) positive pin  →  [220Ω resistor]  →  ESP32 GPIO 4
    (−) negative pin  →  GND rail
```

### Step 11: Status LEDs

```
Red LED (Danger):
    Anode (long leg)   →  [220Ω resistor]  →  ESP32 GPIO 26
    Cathode (short leg) →  GND rail

Green LED (Normal):
    Anode (long leg)   →  [220Ω resistor]  →  ESP32 GPIO 27
    Cathode (short leg) →  GND rail
```

### Step 12: TB6612FNG Motor Driver

```
TB6612FNG Module:
    VM    →  5V bus (motor power)
    VCC   →  ESP32 3V3 (logic reference voltage)
    GND   →  GND rail (both GND pins)
    STBY  →  ESP32 GPIO 5 (HIGH = enabled)

    PWMA  →  ESP32 GPIO 25 (left motor speed PWM)
    AIN1  →  ESP32 GPIO 16 (left direction)
    AIN2  →  ESP32 GPIO 17 (left direction)
    AO1   →  Left motors (+) terminal  ┐
    AO2   →  Left motors (−) terminal  ┘ Front-Left and Rear-Left wired in parallel

    PWMB  →  ESP32 GPIO 14 (right motor speed PWM)
    BIN1  →  ESP32 GPIO 33 (right direction)
    BIN2  →  ESP32 GPIO 2  (right direction) ⚠ see note below
    BO1   →  Right motors (+) terminal  ┐
    BO2   →  Right motors (−) terminal  ┘ Front-Right and Rear-Right wired in parallel
```

> Place a **100nF ceramic capacitor** across VCC and GND on the TB6612 module — solder it or tuck it into the breadboard next to the module.

**Motor wiring — parallel pairs:**
```
        AO1 ───┬──→ Front-Left Motor (+)
               └──→ Rear-Left Motor (+)

        AO2 ───┬──→ Front-Left Motor (−)
               └──→ Rear-Left Motor (−)

        BO1 ───┬──→ Front-Right Motor (+)
               └──→ Rear-Right Motor (+)

        BO2 ───┬──→ Front-Right Motor (−)
               └──→ Rear-Right Motor (−)
```

If the wheels spin the wrong direction, swap the (+) and (−) wires for that motor pair.

### Step 13: Decoupling Capacitor on ESP32

Place a **100nF ceramic capacitor** between ESP32 **3V3** pin and **GND** pin (any GND). This filters high-frequency noise from the motors reaching the ESP32's logic rail.

---

## ⚠ GPIO2 Boot Note

GPIO2 (used for TB6612 BIN2) is a strapping pin. If the motor driver holds it HIGH during power-on, the ESP32 may fail to enter flash mode.

**If you cannot upload firmware:**
1. Disconnect the jumper wire from GPIO2
2. Flash the firmware via USB
3. Reconnect the jumper wire
4. Press the RST button on the ESP32

This only matters during flashing. Once the firmware is running, GPIO2 works fine as an output.

---

## GATEWAY ESP32 — Complete Pin Map

| GPIO | Direction | Connected To | Wire Color |
|---|---|---|---|
| **5V** | Power Out | LCD VCC | Red |
| **GND** | Power | LCD GND + all LED cathodes | Black |
| **21** | I2C SDA | LCD SDA | Green |
| **22** | I2C SCL | LCD SCL | Blue |
| **26** | Output | Red LED anode via 220Ω | Red |
| **27** | Output | Green LED anode via 220Ω | Green |
| **25** | Output | Yellow LED anode via 220Ω | Orange |

---

## GATEWAY — Step-by-Step Wiring

The Gateway is powered entirely by the laptop USB cable. No external power needed.

### Step 1: LCD 16×2 (I2C)

```
LCD I2C Module (4-pin header on the backpack):
    GND  →  ESP32 GND
    VCC  →  ESP32 5V
    SDA  →  ESP32 GPIO 21
    SCL  →  ESP32 GPIO 22
```

> If the LCD shows blocks or is blank, use a small screwdriver to turn the **blue potentiometer** on the back of the I2C backpack. This adjusts contrast.

### Step 2: Status LEDs

```
Red LED (Danger):
    Anode (long leg)   →  [220Ω resistor]  →  ESP32 GPIO 26
    Cathode (short leg) →  GND

Green LED (Normal):
    Anode (long leg)   →  [220Ω resistor]  →  ESP32 GPIO 27
    Cathode (short leg) →  GND

Yellow LED (Heartbeat):
    Anode (long leg)   →  [220Ω resistor]  →  ESP32 GPIO 25
    Cathode (short leg) →  GND
```

That's it for the Gateway. Plug it into the laptop via USB and you're live.

---

## COMPLETE SYSTEM DIAGRAM

```
                    ┌─────────── MINE ───────────┐
                    │                             │
                    │    ┌─── ROVER ESP32 ───┐    │
                    │    │                   │    │
 ┌──────────────┐   │    │  DHT22  → GPIO23  │    │
 │ 10000mAh     │   │    │  MQ-4   → GPIO34  │    │
 │ Power Bank   │───┼───→│  HC-SR04→ GPIO18  │    │
 │ (5V / 2A+)   │   │    │  MPU6050→ I2C     │    │
 └──────────────┘   │    │  VL53L0X→ I2C     │    │
                    │    │  Encoders → 32,35  │    │
                    │    │  Water  → GPIO36   │    │
                    │    │  Servo  → GPIO13   │    │
                    │    │  Motors → TB6612   │    │
                    │    │  Buzzer → GPIO4    │    │
                    │    │  LEDs   → 26,27    │    │
                    │    │                   │    │
                    │    │    ESP-NOW TX      │    │
                    │    └───────┬────────────┘    │
                    │            │                 │
                    └────────────┼─────────────────┘
                                 │
                        ~200m wireless range
                        (no WiFi router needed)
                                 │
                    ┌────────────┼─────────────────┐
                    │            │   BASE DESK      │
                    │    ┌───────▼────────────┐     │
                    │    │  GATEWAY ESP32     │     │
                    │    │                   │     │
                    │    │  ESP-NOW RX       │     │
                    │    │  16×2 LCD (I2C)   │     │
                    │    │  Red/Green/Yellow  │     │
                    │    │  LEDs             │     │
                    │    │                   │     │
                    │    └───────┬────────────┘     │
                    │            │ USB Cable        │
                    │            │ (data + power)   │
                    │    ┌───────▼────────────┐     │
                    │    │  LAPTOP            │     │
                    │    │  Chrome Browser    │     │
                    │    │  dashboard.html    │     │
                    │    │  (Web Serial API)  │     │
                    │    └───────────────────┘     │
                    └─────────────────────────────┘
```

---

## VOLTAGE DIVIDER REFERENCE

Both the HC-SR04 ECHO and MQ-4 AO use identical voltage dividers:

```
5V Signal ──── [10kΩ] ──── Junction ──── [15kΩ] ──── GND
                              │
                         ESP32 GPIO
                     (reads 3.0V max)

    V_out = V_in × R_lower / (R_upper + R_lower)
    V_out = 5.0V × 15kΩ / (10kΩ + 15kΩ)
    V_out = 5.0V × 0.60
    V_out = 3.0V  ← safe for ESP32 (max 3.3V)
```

---

## MOTOR DIRECTION TRUTH TABLE (TB6612FNG)

| IN1 | IN2 | PWM | Motor Action |
|---|---|---|---|
| HIGH | LOW | 0–255 | Forward (speed = PWM duty) |
| LOW | HIGH | 0–255 | Reverse (speed = PWM duty) |
| HIGH | HIGH | 255 | Short brake (motor locked) |
| LOW | LOW | any | Coast (free spin) |

STBY must be HIGH for the driver to operate. STBY LOW = all outputs disabled (standby mode).

---

## FIRST POWER-ON CHECKLIST

Before applying power, verify:

```
[ ] 470µF cap on 5V bus (correct polarity — long leg to +5V)
[ ] 100nF cap on ESP32 3V3-GND
[ ] 100nF cap on TB6612 VCC-GND
[ ] MQ-4 VCC on 5V bus (NOT 3V3)
[ ] MQ-4 AO goes through 10k/15k divider to GPIO34 (NOT direct)
[ ] HC-SR04 ECHO goes through 10k/15k divider to GPIO18 (NOT direct)
[ ] HC-SR04 VCC on 5V bus
[ ] Servo V+ on 5V bus (NOT ESP32 5V pin)
[ ] TB6612 VM on 5V bus
[ ] All GND wires connected to the same GND bus
[ ] No bare wire ends touching each other
[ ] DHT22 on 3V3 (not 5V)
[ ] Water sensor on 3V3 (not 5V)
[ ] LED resistors (220Ω) are present (no direct GPIO → LED)
```

> **MQ-4 warm-up:** The MQ-4 sensor needs 24–48 hours of continuous power for the heater to stabilize on first use. Readings in the first few minutes will be unreliable. For the hackathon demo, power it on as early as possible.
