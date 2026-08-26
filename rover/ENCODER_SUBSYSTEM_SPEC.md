# DEEPTRACK Rover — Dual LM393 Encoder & Odometry Subsystem Specification
**Document Version:** 2.0
**Target Audience:** Firmware Engineering (Codex / Production Firmware Implementation)
**Scope:** Architecture, Interrupt Handling, Signal Conditioning, Dynamic Piecewise Calibration, and Closed-Loop Sensor Fusion for `DeeptrackRover` Production Firmware.

---

## 1. Executive Summary & Core Constraints

During hardware diagnostics on the physical DEEPTRACK rover, multiple severe electrical, physical, and optical anomalies were analyzed and resolved:
1. **Asymmetric Gearbox & Optical Sensitivity:** Left encoder registers $\sim 2.5\times$ to $6\times$ more pulses than the right encoder under identical commanded PWM.
2. **Motor Driver Switching Saturation:** The L298N driver stalls below PWM 200 when driven at high PWM frequencies ($\ge 18\text{ kHz}$). **Motor PWM frequency MUST be set to $1000\text{ Hz}$ ($1\text{ kHz}$)**.
3. **Open-Collector Signal Dynamics:** ESP32 pins `GPIO 34` and `GPIO 35` are **Input-Only with NO internal pull-up resistors**. LM393 open-collector pull-ups produce slow $RC$ rise times but razor-sharp pull-downs. **Interrupts MUST trigger on `FALLING` edge**, not `RISING`.
4. **Pending Ground-Weight Calibration:** Calibration parameters will shift once the full chassis payload (ThinkPad 3S battery + sensors) is mounted. **All calibration scalars, base ratios, and control gains MUST be exposed in an accessible configuration layer / NVS Preferences, NEVER hardcoded as inline magic numbers.**

---

## 2. Hardware Realities & Signal Conditioning

### 2.1 Pin Assignment & Interrupt Mode
- **Left Encoder Pin:** `GPIO 34` (Input only, external pull-up required).
- **Right Encoder Pin:** `GPIO 35` (Input only, external pull-up required).
- **Interrupt Trigger:** **`FALLING` edge strictly**.
  - *Rationale:* LM393 comparator pulls to GND actively. If `RISING` is used, weak pull-ups cause sluggish rise times at high RPM, leading to massive missed pulse dropouts.

### 2.2 Temporal Debounce Filtering ($1500\,\mu\text{s}$)
At higher RPM, optical disk flutter and chassis vibration cause the slotted optical beam to chatter across the slot edge, producing burst pulses ($< 800\,\mu\text{s}$ apart).
- **Enforce Minimum Pulse Interval:** `ENCODER_MIN_PULSE_US = 1500` ($1.5\text{ ms}$).
- A $1.5\text{ ms}$ debounce supports up to $666\text{ ticks/sec}$ (far above physical 20-slot TT motor limits of $\sim 150\text{ ticks/sec}$) while rejecting 100% of optical chatter.

### 2.3 Motor-State Gating (Cross-talk Elimination)
Electrical noise from one motor's brushes can induce false pulses on the opposing encoder line.
- **Rule:** Filtered odometry ticks are only incremented if the corresponding motor is actively commanded:
  $$|\text{motorCommand}| \ge \text{MOTOR\_ACTIVE\_THRESHOLD} \quad (\text{Threshold} = 25\text{ PWM})$$
- Maintain **4 distinct counters**:
  - `leftRawTicks`, `rightRawTicks` (for low-level hardware diagnostics).
  - `leftFilteredTicks`, `rightFilteredTicks` (for odometry, distance, and balance).

### 2.4 Reference ISR Implementation
```cpp
// Volatile globals
volatile uint32_t leftRawTicks = 0, rightRawTicks = 0;
volatile uint32_t leftFilteredTicks = 0, rightFilteredTicks = 0;
volatile uint32_t lastLeftEncoderUs = 0, lastRightEncoderUs = 0;
volatile int leftMotorCommand = 0, rightMotorCommand = 0;

constexpr uint32_t ENCODER_MIN_PULSE_US = 1500;
constexpr int MOTOR_ACTIVE_THRESHOLD = 25;

void IRAM_ATTR onLeftEncoder() {
    const uint32_t now = micros();
    if (now - lastLeftEncoderUs < ENCODER_MIN_PULSE_US) return;
    lastLeftEncoderUs = now;
    ++leftRawTicks;
    const int cmd = leftMotorCommand;
    if (cmd >= MOTOR_ACTIVE_THRESHOLD || cmd <= -MOTOR_ACTIVE_THRESHOLD) {
        ++leftFilteredTicks;
    }
}

void IRAM_ATTR onRightEncoder() {
    const uint32_t now = micros();
    if (now - lastRightEncoderUs < ENCODER_MIN_PULSE_US) return;
    lastRightEncoderUs = now;
    ++rightRawTicks;
    const int cmd = rightMotorCommand;
    if (cmd >= MOTOR_ACTIVE_THRESHOLD || cmd <= -MOTOR_ACTIVE_THRESHOLD) {
        ++rightFilteredTicks;
    }
}
```

---

## 3. Dynamic Piecewise Scaling & Calibration Architecture

### 3.1 The Dual-Regime Non-Linearity Problem
Hardware test data reveals two distinct operating regimes:
- **Low-Speed Regime ($\text{PWM} < 185$):** Effective ratio $\approx 0.40 - 0.45$.
- **High-Speed Regime ($\text{PWM} \ge 185$):** Effective ratio $\approx 0.16 - 0.18$.
- **On-Floor Loaded Regime (Physical Weight):** Effective ratio $\approx 0.4000$.

Using a single static scale factor causes severe over-steering or under-steering whenever speed changes.

### 3.2 Accessible Configuration Layer (NVS / Preferences)
Codex must encapsulate all encoder configuration into a structured configuration block backed by ESP32 `Preferences` (NVS namespace `"deeptrack"` or `"rover_cfg"`):

```cpp
struct EncoderConfig {
    float leftScaleCruise   = 0.6078f;  // High-speed cruise scale factor (NVS: "enc_scale_l")
    float rightScale        = 1.0000f;  // Right side reference scale (NVS: "enc_scale_r")
    float lowSpeedMultiplier= 1.0000f;  // Low-speed boost multiplier (NVS: "enc_low_mult")
    float kpEncoder         = 2.2000f;  // Encoder rate balance gain (NVS: "enc_kp")
    float kpGyro            = 25.000f;  // Gyro Z yaw-rate damping gain (NVS: "gyro_kp")
    int   maxBalanceCorr    = 30;       // Max PWM trim authority (NVS: "bal_max_corr")
    uint32_t windowMs       = 100;      // Balance loop period in ms (NVS: "bal_win_ms")
};
```

### 3.3 Dynamic Scale Lookup Function
```cpp
float getEffectiveLeftScale(int commandedPwm, const EncoderConfig &cfg) {
    const int absPwm = abs(commandedPwm);
    if (absPwm < 185) {
        return cfg.leftScaleCruise * cfg.lowSpeedMultiplier;
    }
    return cfg.leftScaleCruise;
}
```

### 3.4 Distance Calculation
```cpp
constexpr float WHEEL_DIAMETER_CM   = 6.5f;
constexpr float LEFT_TICKS_PER_REV  = 20.0f;
constexpr float RIGHT_TICKS_PER_REV = 20.0f;

float odometryDistanceCm(const EncoderConfig &cfg) {
    constexpr float cmPerTickL = (PI * WHEEL_DIAMETER_CM) / LEFT_TICKS_PER_REV;
    constexpr float cmPerTickR = (PI * WHEEL_DIAMETER_CM) / RIGHT_TICKS_PER_REV;
    const float effScale = getEffectiveLeftScale(leftMotorCommand, cfg);

    const float distL = leftFilteredTicks  * effScale       * cmPerTickL;
    const float distR = rightFilteredTicks * cfg.rightScale * cmPerTickR;

    const bool lActive = abs(leftMotorCommand)  >= MOTOR_ACTIVE_THRESHOLD;
    const bool rActive = abs(rightMotorCommand) >= MOTOR_ACTIVE_THRESHOLD;

    // Degraded fallback if one sensor stalls
    if (lActive && leftFilteredTicks == 0 && rightFilteredTicks > 0) return distR;
    if (rActive && rightFilteredTicks == 0 && leftFilteredTicks > 0) return distL;
    return (distL + distR) / 2.0f;
}
```

---

## 4. Closed-Loop Straight-Line Balance & Gyro Z Fusion

### 4.1 Anti-Windup Proportional Control (Crucial Bug Fix)
**DO NOT** accumulate corrections inside a static/global accumulator (`corr = corr + delta`). That creates an unbounded integrator that rails to $\pm \text{MAX\_CORR}$ within 3 seconds of driving.
Corrections must be **proportional to the instantaneous rate error and yaw rate**.

### 4.2 Mathematical Sensor Fusion Model
Every $100\text{ ms}$ ($10\text{ Hz}$ loop):
1. **Normalized Encoder Error:**
   $$e_{\text{enc}} = (\Delta \text{leftFiltered} \times \text{effLeftScale}) - (\Delta \text{rightFiltered} \times \text{rightScale})$$
2. **Gyroscope Z Yaw Rate:**
   $$\omega_z = \text{gyro.z} - \text{gyroBiasZ} \quad (\text{rad/sec})$$
3. **Blended Dynamic Trim Calculation:**
   $$\text{targetTrim} = (\omega_z \times K_{\text{gyro}}) - (e_{\text{enc}} \times K_{\text{enc}})$$
   $$\text{trimPwm} = \text{clamp}(\text{round}(\text{targetTrim}), -\text{maxCorr}, +\text{maxCorr})$$
   $$\text{leftCorrection} = +\text{trimPwm}, \quad \text{rightCorrection} = -\text{trimPwm}$$

### 4.3 Drive Application
During straight forward or reverse driving (`CRUISE` or manual `forward`):
$$\text{Applied PWM}_L = \text{clamp}(\text{targetSpeed} + \text{leftCorrection}, 120, 255)$$
$$\text{Applied PWM}_R = \text{clamp}(\text{targetSpeed} + \text{rightCorrection}, 120, 255)$$

During intentional steering, pivot turns, or obstacle avoidance:
$$\text{leftCorrection} = 0, \quad \text{rightCorrection} = 0$$

---

## 5. Automated Multi-Point Calibration Routine (`enccal`)

Codex should provide an automated calibration command in the CLI.

### 5.1 Routine Protocol
1. **Pre-requisite:** Warn user to lift wheels off ground (5-second countdown).
2. **Breakaway Kick-Start:** For each test PWM level ($\{150, 165, 180, 195, 210, 220, 230, 240, 250, 255\}$), pulse the motor at PWM 255 for $50\text{ ms}$ to overcome static gearbox friction, then zero counters and measure steady state for $2000\text{ ms}$.
3. **Calculate Per-Step Ratio:** $\text{Ratio}_i = \frac{\text{RightTicks}_i}{\text{LeftTicks}_i}$.
4. **Statistical Aggregation:** Sort ratios, compute Median Ratio and Standard Deviation.
5. **Persist to NVS:** Save median ratio to `enc_scale_l` and `1.000` to `enc_scale_r`.

---

## 6. Serial CLI & Runtime Adjustment Commands

Codex must support the following runtime commands for on-the-fly field calibration:
- `enc` — Outputs raw ticks, filtered ticks, active effective scale, normalized tick counts, tick rate (Hz), and live trim corrections.
- `resetenc` — Resets all raw/filtered counters, re-arms interrupts to `FALLING`.
- `enccal` — Runs automated 10-point speed sweep calibration.
- `encscale left <float>` — Immediately updates and persists `leftScaleCruise` in NVS.
- `encscale right <float>` — Updates and persists `rightScale` in NVS.
- `keepalive on/off` — Enables/disables periodic 50ms load pulse to prevent USB power bank auto-sleep.

---

## 7. Codex Implementation Checklist

- [ ] Set motor PWM frequency to **$1000\text{ Hz}$** in `DeeptrackHardware.h` / motor driver setup.
- [ ] Configure encoder interrupts to **`FALLING`** mode on `GPIO 34` and `GPIO 35`.
- [ ] Implement $1500\,\mu\text{s}$ software debounce in ISRs.
- [ ] Implement $|\text{cmd}| \ge 25$ motor-command gating for filtered counters.
- [ ] Create `EncoderConfig` struct backed by ESP32 `Preferences` (NVS) for all calibration variables.
- [ ] Implement `getEffectiveLeftScale()` with piecewise/dual-regime scaling.
- [ ] Implement $10\text{ Hz}$ balance loop fusing **MPU Gyro Z-axis** ($\omega_z$) and **normalized encoder rate** ($e_{\text{enc}}$) without accumulator windup.
- [ ] Implement CLI commands: `enc`, `resetenc`, `enccal`, `encscale left <val>`, `encscale right <val>`.
- [ ] Conduct final ground-weight calibration once chassis payload is fully assembled.
