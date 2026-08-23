# DEEPTRACK — Mine Rescue Rover & Gateway System Guide

Complete instructions for simulating both circuits in Wokwi today and running on physical hardware tomorrow.

---

## 🏗️ System Architecture

```
+---------------------+                      +----------------------+                      +-----------------------+
|     ROVER ESP32     |                      |    GATEWAY ESP32     |                      |   LAPTOP DASHBOARD    |
|   (Inside Mine)     |                      |     (Base Desk)      |                      |   (Browser Console)   |
|                     |                      |                      |                      |                       |
| - Environmental Sen |  ESP-NOW (Hardware)  | - 16x2 I2C LCD       |  USB Serial (115200) | - Live Gauge Meters   |
| - Obstacle Avoidance| ===================> | - Status LEDs        | -------------------> | - Real-time Path Map  |
| - Odometry Engine   |  or socat TCP Bridge | - Heartbeat Monitor  |        AND / OR      | - Danger Alarms & Log |
| - Motor Actuators   |                      | - Serial Relay       |   MQTT WebSocket     | - Multi-source Toggle |
|                     |                      | - Hot-Pluggable MQTT | ===(HiveMQ Broker)==>|   (MQTT / Serial /    |
+---------------------+                      +----------------------+                      |    Demo Mode)         |
                                                                                           +-----------------------+
```

---

## 📂 Clean Project Structure

```text
jharkhand/
├── src/main.cpp              # Rover ESP32 firmware (Sensors, Motors, ESP-NOW TX)
├── include/telemetry_packet.h# Shared 48-byte packed telemetry struct
├── diagram.json              # Rover Wokwi simulation circuit
├── platformio.ini            # Rover PlatformIO build configuration
├── wokwi.toml                # Rover Wokwi configuration (RFC2217 port: 4000)
│
├── GATEWAY/
│   ├── src/main.cpp          # Gateway ESP32 firmware (ESP-NOW RX, LCD, Serial Relay)
│   ├── include/
│   │   └── telemetry_packet.h# Shared telemetry struct (exact copy)
│   ├── diagram.json          # Gateway Wokwi simulation circuit (ESP32 + LCD + LEDs)
│   ├── platformio.ini        # Gateway PlatformIO build configuration
│   └── wokwi.toml            # Gateway Wokwi configuration (RFC2217 port: 4001)
│
├── DASHBOARD/
│   └── dashboard.html        # Web dashboard (Web Serial connection, path trace)
│
├── docs/                     # Schematics, netlists & reference documents
│   ├── cirkit_netlist.json
│   ├── cirkit_rover_circuit.md
│   └── LATER_SCOPE/
│       └── DESIGN.md
├── GUIDE.md                  # This operational guide
└── .gitignore                # Clean build artifact filters
```

---

## 💻 Part 1: Running Both Circuits in Wokwi Simulation

Wokwi runs each microcontroller in its own isolated simulation. Using **RFC2217 Serial Forwarding** and Linux `socat`, both simulations talk to each other in real time.

### Step 1: Install `socat` (if not already installed)
```bash
sudo apt update && sudo apt install -y socat
```

### Step 2: Build both firmware projects
```bash
# Build Rover firmware
cd /home/dipak/code/jharkhand
pio run

# Build Gateway firmware
cd /home/dipak/code/jharkhand/GATEWAY
pio run
```

### Step 3: Start the Virtual Serial Bridge
Open a terminal and run:
```bash
socat -u tcp:localhost:4000 tcp:localhost:4001
```
*(Leave this terminal running. It forwards telemetry output from Rover port 4000 directly to Gateway port 4001).*

### Step 4: Start Both Simulations in VS Code
1. Open the Rover project (`/home/dipak/code/jharkhand`) in VS Code. Press `F1` or `Ctrl+Shift+P` -> select **Wokwi: Start Simulator**.
2. Open the Gateway project (`/home/dipak/code/jharkhand/GATEWAY`) in a second VS Code window/tab. Select **Wokwi: Start Simulator**.

### Step 5: Connect Gateway to the Dashboard
1. Open [`DASHBOARD/dashboard.html`](file:///home/dipak/code/jharkhand/DASHBOARD/dashboard.html) in Google Chrome, Microsoft Edge, or Chromium.
2. In the Gateway Wokwi serial tab, the gateway receives the Rover's telemetry and prints JSON to its USB Serial.
3. If you want to preview the dashboard independently, click **Demo Mode** on the dashboard.

---

## ⚡ Part 2: Quick Gateway Standalone Sim (No Bridge Needed)

If you just want to test the Gateway LCD and LEDs in Wokwi without running the Rover:
1. Open `GATEWAY/` in VS Code and start the Wokwi simulation.
2. The Gateway displays `"DEEPTRACK GTWRY / Waiting link..."`.
3. After **5 seconds** without incoming packets, the Gateway **automatically enters Demo Mode**.
4. The 16x2 LCD will begin cycling through `ENV`, `NAV`, and `STATUS` screens every 3 seconds, and the yellow heartbeat LED will blink on every cycle.

---

## 🚀 Part 3: Running on Physical Hardware (Tomorrow)

On real hardware, no internet, no router, and no `socat` bridge are required. Both ESP32s communicate peer-to-peer using **ESP-NOW** at ~200m range.

### Step 1: Flash the Rover ESP32
1. Plug the Rover ESP32 into your laptop via USB.
2. Flash the firmware:
   ```bash
   cd /home/dipak/code/jharkhand
   pio run -t upload
   ```
3. Disconnect Rover ESP32 and power it using your battery pack.

### Step 2: Flash the Gateway ESP32
1. Plug the Gateway ESP32 into your laptop via USB.
2. Flash the firmware:
   ```bash
   cd /home/dipak/code/jharkhand/GATEWAY
   pio run -t upload
   ```
3. **Leave the Gateway ESP32 plugged into the laptop via USB.**

### Step 3: View Live Telemetry
1. **On the Gateway 16x2 LCD**: The screen will instantly display live Rover telemetry:
   - **Screen 1 (ENV)**: Temperature (°C), Humidity (%), MQ Gas reading, Water sensor reading.
   - **Screen 2 (NAV)**: Relative position (X, Y cm), Heading angle, Ultrasonic front distance.
   - **Screen 3 (STATUS)**: Rover state (`NORMAL`, `SLOW`, `AVOIDING`), Link status (`OK`), Packet count.
   - **DANGER Screen (Override)**: In hazard conditions, locks to `!! DANGER !!` with cause (e.g. `GAS`, `WATER`, `TILT`).
2. **On the Gateway Status LEDs**:
   - 🟢 **Green LED**: Normal operation & active link.
   - 🔴 **Red LED**: Danger state active (gas leak, water flood, severe tilt, or trapped).
   - 🟡 **Yellow LED**: Blinks on every incoming telemetry packet (heartbeat).

### Step 4: Stream Live Data to the Laptop Dashboard (Hot-Pluggable)
Open [`DASHBOARD/dashboard.html`](file:///home/dipak/code/jharkhand/DASHBOARD/dashboard.html) in Chrome or Edge. You have two live connection modes:

* **Mode A: Connect Serial (Direct USB Cable)**
  1. Click the green **Connect Serial** button.
  2. Select the Gateway's USB COM port (`/dev/ttyUSB0` or `COMx`).
  3. Status turns **Serial Live** — gauges, path trace, and logs update instantly.

* **Mode B: Connect MQTT (Wireless / Cloud Broker)**
  1. Click the amber **Connect MQTT** button.
  2. Subscribes over WebSockets to HiveMQ on topic `deeptrack/rover/telemetry`.
  3. Status turns **MQTT Live** — data published by the Gateway streams over the internet.

* **Mode C: Demo Mode**
  1. Click **Demo Mode** to test animations and gauges with synthetic rover data.

---

## 📌 Hardware Wiring Reference

### Gateway ESP32 Pinout

| Peripheral | ESP32 Pin | Details |
|---|---|---|
| **16x2 LCD SDA** | `GPIO 21` | I2C Data (Address `0x27`) |
| **16x2 LCD SCL** | `GPIO 22` | I2C Clock |
| **16x2 LCD VCC / GND** | `5V` / `GND` | Power |
| **Red LED (Danger)** | `GPIO 26` | Via 220Ω resistor to Anode, Cathode to GND |
| **Green LED (Normal)** | `GPIO 27` | Via 220Ω resistor to Anode, Cathode to GND |
| **Yellow LED (Heartbeat)**| `GPIO 25` | Via 220Ω resistor to Anode, Cathode to GND |

### Rover ESP32 Pinout

| Peripheral | ESP32 Pin | Details |
|---|---|---|
| **DHT22 Data** | `GPIO 23` | Temperature & Humidity Sensor |
| **MQ-4 Methane Sensor (AO)** | `GPIO 34` | Analog input (via 10k/15k divider if 5V) |
| **HC-SR04 Trigger** | `GPIO 19` | Ultrasonic distance trigger |
| **HC-SR04 Echo** | `GPIO 18` | Ultrasonic distance echo (via divider to 3.3V) |
| **SG90 Scan Servo** | `GPIO 13` | Mounts ultrasonic sensor for sweeping |
| **MPU6050 SDA / SCL** | `GPIO 21` / `GPIO 22` | I2C Gyro / Accelerometer (Tilt) |
| **Left Wheel Encoder** | `GPIO 32` | Interrupt pulse counter |
| **Right Wheel Encoder**| `GPIO 35` | Interrupt pulse counter |
| **VL53L0X ToF (or Sim)**| `GPIO 39 (VN)` | Secondary obstacle sensor |
| **Water Level Sensor** | `GPIO 36 (VP)` | Analog flood detection |
| **Left Motor PWM** | `GPIO 25` | Motor driver PWM (LEDC CH 0) |
| **Right Motor PWM** | `GPIO 14` | Motor driver PWM (LEDC CH 1) |
| **Buzzer** | `GPIO 4` | Audio alarm |
| **Red / Green LEDs** | `GPIO 26` / `GPIO 27` | Rover onboard indicators |

---

## 🛠️ Troubleshooting

- **Linux Serial Port Permission**:
  If Chrome or PlatformIO shows `Permission Denied` on `/dev/ttyUSB0`:
  ```bash
  sudo usermod -a -G dialout $USER
  ```
  *(Log out and log back in for changes to take effect).*
- **LCD Shows Black Blocks or Blank Screen**:
  Adjust the small blue potentiometer on the back of the I2C backpack with a screwdriver to adjust LCD contrast.
- **ESP-NOW Range**:
  ESP-NOW operates on 2.4GHz WiFi channels and easily reaches 150-200 meters in line of sight without requiring any WiFi router.
