## 1. complete project architecture

tumhare project mein do independent ESP32 boards hain:

| module              | role                                   | power                                  |
| ------------------- | -------------------------------------- | -------------------------------------- |
| rover ESP32         | motors, sensors, servo, LEDs, buzzer   | 2×18650 battery → buck converter → 5 V |
| gateway ESP32       | laptop communication, LCD, status LEDs | laptop USB                             |
| rover motor drivers | chaar TT motors control karte hain     | separate 4×AA NiMH battery pack        |
| rover ↔ gateway     | ESP-NOW wireless communication         | koi physical data wire nahi            |

rover ke andar logic battery aur motor battery ka **ground common** hoga. unke **positive terminals separate** rahenge.

---

## 2. tumhare actual ESP32 board ka complete physical pin map

board ko aise rakho ki **USB-C port upar** ho.

| left side, top → bottom | actual GPIO | assigned connection             | right side, top → bottom | actual GPIO | assigned connection                      |
| ----------------------- | ----------: | ------------------------------- | ------------------------ | ----------: | ---------------------------------------- |
| `3V3`                   |           — | 3.3 V power rail                | `VIN`                    |           — | regulated 5 V input                      |
| `GND`                   |           — | common ground                   | `GND`                    |           — | common ground                            |
| `D15`                   |          15 | unused; boot-sensitive          | `D13`                    |          13 | servo signal                             |
| `D2`                    |           2 | unused; boot-sensitive          | `D12`                    |          12 | unused; boot-sensitive                   |
| `D4`                    |           4 | buzzer control                  | `D14`                    |          14 | left motor direction 1                   |
| `RX2`                   |          16 | left motor direction 2          | `D27`                    |          27 | green rover LED                          |
| `TX2`                   |          17 | right motor PWM                 |                          |             |                                          |
| `D26`                   |          26 | red rover LED                   |                          |             |                                          |
| `D5`                    |           5 | unused; boot-sensitive          | `D25`                    |          25 | left motor PWM                           |
| `D18`                   |          18 | HC-SR04 `ECHO`, through divider | `D33`                    |          33 | right motor direction 1                  |
| `D19`                   |          19 | HC-SR04 `TRIG`                  | `D32`                    |          32 | right motor direction 2                  |
| `D21`                   |          21 | I²C `SDA`                       | `D35`                    |          35 | right encoder                            |
| `RX0`                   |           3 | unused; USB serial              | `D34`                    |          34 | left encoder                             |
| `TX0`                   |           1 | unused; USB serial              | `VN`                     |          39 | water sensor analog output               |
| `D22`                   |          22 | I²C `SCL`                       | `VP`                     |          36 | MQ-4 analog output, through divider      |
| `D23`                   |          23 | DHT22 data                      | `EN`                     |           — | reset/enable; do not connect peripherals |

sabse confusing aliases:

| board label | firmware GPIO |
| ----------- | ------------: |
| `RX2`       |          `16` |
| `TX2`       |          `17` |
| `VP`        |          `36` |
| `VN`        |          `39` |

`D34`, `D35`, `VP`, aur `VN` **input-only** hain. `D34` aur `D35` mein internal pull-up bhi nahi hai. `D2`, `D5`, `D12`, aur `D15` boot-sensitive hain. [Espressif GPIO documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html)

**physical issue:** tumhari photo mein ESP32 par male header pins soldered nahi dikh rahe. jumper wires/breadboard ke liye **2×15 male header pins solder** karne honge.

---

## 3. rover ke chaar electrical rails

wiring organize karne ke liye chaar named rails samjho:

| rail         | voltage                 | source                                   | connected components                                          |
| ------------ | ----------------------- | ---------------------------------------- | ------------------------------------------------------------- |
| `LOGIC_5V`   | 5.0 V                   | buck converter output                    | ESP32 `VIN`, MQ-4, HC-SR04, SG90, buzzer if 5 V               |
| `LOGIC_3V3`  | approximately 3.3 V     | ESP32 `3V3` pin                          | DHT22, MPU6050, VL53L0X, encoders, water sensor, TB6612 logic |
| `MOTOR_POS`  | approximately 4.8–5.6 V | 4×AA NiMH pack                           | both TB6612 `VM` pins                                         |
| `COMMON_GND` | 0 V                     | logic-ground and motor-ground connection | ESP32, sensors, drivers, servo, both battery systems          |

`LOGIC_5V` aur `MOTOR_POS` ko **aapas mein connect mat karna**.

---

## 4. 2×18650 logic battery, BMS, buck converter

### 4.1 2S battery configuration

2 lithium cells series mein:

| battery connection                                  | destination                                   |
| --------------------------------------------------- | --------------------------------------------- |
| first cell negative                                 | BMS `B-`                                      |
| first cell positive                                 | second cell negative                          |
| first-cell-positive / second-cell-negative junction | BMS `B1`, `BM`, or equivalent middle terminal |
| second cell positive                                | BMS `B+`                                      |

typical 2S pack voltage:

| battery state            | approximate voltage |
| ------------------------ | ------------------: |
| fully charged            |               8.4 V |
| nominal                  |               7.4 V |
| significantly discharged | approximately 6–7 V |

**important:** exact BMS labels model-dependent hote hain. `B-`, middle-cell connection, aur `B+` verify kiye bina connect mat karna.

### 4.2 BMS output to buck converter

| from                                                | to                   | note                   |
| --------------------------------------------------- | -------------------- | ---------------------- |
| BMS `P+`, or designated protected positive terminal | switch input         | optional master switch |
| switch output                                       | buck converter `IN+` | logic supply positive  |
| BMS `P-`                                            | buck converter `IN-` | protected logic ground |
| buck converter `OUT+`                               | `LOGIC_5V` rail      | adjust to 5.0 V        |
| buck converter `OUT-`                               | `COMMON_GND`         | common ground          |

agar switch nahi hai, BMS protected positive directly buck `IN+` par connect hoga.

**BMS `B-` ko directly load ground ki tarah use mat karna** jab BMS ka designated output `P-` ho. warna protection bypass ho sakti hai.

### 4.3 buck converter adjustment

1. ESP32 aur sensors connect karne se pehle buck converter ko battery se power do.
2. multimeter ko DC voltage mode par set karo.
3. black probe `OUT-`, red probe `OUT+`.
4. adjustment screw turn karke output **5.0 V** set karo.
5. uske baad hi ESP32 `VIN` connect karo.

| buck converter terminal | final connection              |
| ----------------------- | ----------------------------- |
| `IN+`                   | protected 2S battery positive |
| `IN-`                   | protected 2S battery negative |
| `OUT+`                  | rover `LOGIC_5V`              |
| `OUT-`                  | rover `COMMON_GND`            |

buck ideally **3 A-capable** hona chahiye because ESP32, MQ-4 heater, aur servo simultaneously current demand kar sakte hain.

**8.4 V battery directly ESP32 `VIN` par mat lagana.**

### 4.4 charging

2S lithium pack ke liye **8.4 V CC/CV charger** chahiye, compatible 2S BMS ke saath.

single-cell `TP4056` module ko complete 2S pack charge karne ke liye directly use nahi karna.

---

## 5. 4×AA motor battery

| motor battery terminal | connect to        |
| ---------------------- | ----------------- |
| 4×AA pack positive     | left TB6612 `VM`  |
| 4×AA pack positive     | right TB6612 `VM` |
| 4×AA pack negative     | `COMMON_GND`      |

agar motor power switch hai:

| from                   | to                          |
| ---------------------- | --------------------------- |
| motor battery positive | motor switch input          |
| motor switch output    | both motor-driver `VM` pins |
| motor battery negative | `COMMON_GND`                |

**critical connection:**

| wire                   | connection                  |
| ---------------------- | --------------------------- |
| motor battery negative | buck `OUT-` / ESP32 `GND`   |
| motor battery positive | only both motor-driver `VM` |
| buck `OUT+`            | only `LOGIC_5V`             |

motor battery positive aur buck 5 V positive alag rahenge.

motor supply wires soldered connections ya screw terminals se distribute karo. chaar motors ka current ordinary breadboard rail se pass karna reliable nahi hai.

---

## 6. ESP32 power connections

| ESP32 printed pin | connect to                         |
| ----------------- | ---------------------------------- |
| `VIN`             | buck converter `OUT+`, exactly 5 V |
| either `GND`      | buck converter `OUT-`              |
| either `GND`      | motor battery negative             |
| `3V3`             | 3.3 V components ki common supply  |

`VIN` aur USB-C se board ko simultaneously power mat karo unless tumhare specific board ki power-path protection verified ho.

program upload karte waqt safer sequence:

1. buck ka `VIN` connection temporarily disconnect karo.
2. USB-C se firmware upload karo.
3. USB disconnect karo.
4. buck ka 5 V output wapas `VIN` se connect karo.

---

## 7. left TB6612FNG: left-front + left-rear

is board ke channel A aur B ko same left-side control signals milenge, lekin **har motor apne separate channel** par rahegi.

| left driver pin | ESP32 board label / destination | actual GPIO |
| --------------- | ------------------------------- | ----------: |
| `VCC`           | ESP32 `3V3`                     |           — |
| `VM`            | 4×AA motor battery positive     |           — |
| `GND`           | `COMMON_GND`                    |           — |
| `STBY`          | ESP32 `3V3`                     |           — |
| `PWMA`          | ESP32 `D25`                     |          25 |
| `AIN1`          | ESP32 `D14`                     |          14 |
| `AIN2`          | ESP32 `RX2`                     |          16 |
| `AO1` / `A01`   | left-front motor terminal 1     |           — |
| `AO2` / `A02`   | left-front motor terminal 2     |           — |
| `PWMB`          | ESP32 `D25`                     |          25 |
| `BIN1`          | ESP32 `D14`                     |          14 |
| `BIN2`          | ESP32 `RX2`                     |          16 |
| `BO1` / `B01`   | left-rear motor terminal 1      |           — |
| `BO2` / `B02`   | left-rear motor terminal 2      |           — |

matlab physically:

| ESP32 pin | fan-out                     |
| --------- | --------------------------- |
| `D25`     | left-driver `PWMA` + `PWMB` |
| `D14`     | left-driver `AIN1` + `BIN1` |
| `RX2`     | left-driver `AIN2` + `BIN2` |

---

## 8. right TB6612FNG: right-front + right-rear

| right driver pin | ESP32 board label / destination | actual GPIO |
| ---------------- | ------------------------------- | ----------: |
| `VCC`            | ESP32 `3V3`                     |           — |
| `VM`             | 4×AA motor battery positive     |           — |
| `GND`            | `COMMON_GND`                    |           — |
| `STBY`           | ESP32 `3V3`                     |           — |
| `PWMA`           | ESP32 `TX2`                     |          17 |
| `AIN1`           | ESP32 `D33`                     |          33 |
| `AIN2`           | ESP32 `D32`                     |          32 |
| `AO1` / `A01`    | right-front motor terminal 1    |           — |
| `AO2` / `A02`    | right-front motor terminal 2    |           — |
| `PWMB`           | ESP32 `TX2`                     |          17 |
| `BIN1`           | ESP32 `D33`                     |          33 |
| `BIN2`           | ESP32 `D32`                     |          32 |
| `BO1` / `B01`    | right-rear motor terminal 1     |           — |
| `BO2` / `B02`    | right-rear motor terminal 2     |           — |

physically:

| ESP32 pin | fan-out                      |
| --------- | ---------------------------- |
| `TX2`     | right-driver `PWMA` + `PWMB` |
| `D33`     | right-driver `AIN1` + `BIN1` |
| `D32`     | right-driver `AIN2` + `BIN2` |

**important distinction:**

* `VCC` = driver logic supply = **3.3 V**.
* `VM` = motor supply = **motor battery voltage**.
* `STBY` = **3.3 V**, otherwise driver remains disabled.

driver inputs internally pulled down hote hain, aur TB6612 mein channel-specific current handling hoti hai. har motor ko separate channel dena correct arrangement hai. [Toshiba TB6612FNG datasheet](https://toshiba.semicon-storage.com/info/docget.jsp?did=10660)

---

## 9. four TT motors

| motor       | driver       | driver channel | motor wires   |
| ----------- | ------------ | -------------- | ------------- |
| left-front  | left TB6612  | channel A      | `AO1` + `AO2` |
| left-rear   | left TB6612  | channel B      | `BO1` + `BO2` |
| right-front | right TB6612 | channel A      | `AO1` + `AO2` |
| right-rear  | right TB6612 | channel B      | `BO1` + `BO2` |

TT motor ke dono terminals par koi fixed universal polarity nahi hoti. agar forward command par koi ek wheel reverse ghoom raha hai, **sirf us wheel ke dono motor wires interchange** kar do.

encoder discs ek left wheel aur ek right wheel par mount karo.

---

## 10. DHT22 temperature/humidity sensor

### three-pin module

| DHT22 module pin     | connect to   |
| -------------------- | ------------ |
| `VCC` / `+`          | ESP32 `3V3`  |
| `DATA` / `OUT` / `S` | ESP32 `D23`  |
| `GND` / `−`          | `COMMON_GND` |

firmware pin: `GPIO23`.

three-pin module mein pull-up generally already installed hota hai.

### bare four-pin DHT22

agar bare sensor hai:

| DHT22 pin | connect to   |
| --------- | ------------ |
| `VCC`     | ESP32 `3V3`  |
| `DATA`    | ESP32 `D23`  |
| `NC`      | disconnected |
| `GND`     | `COMMON_GND` |

additional resistor:

| resistor | first end      | second end  |
| -------- | -------------- | ----------- |
| 10 kΩ    | `DATA` / `D23` | ESP32 `3V3` |

---

## 11. MQ-4 methane sensor

tumhara MQ-4 four-pin module hai.

| MQ-4 pin | connect to                   |
| -------- | ---------------------------- |
| `VCC`    | `LOGIC_5V`                   |
| `GND`    | `COMMON_GND`                 |
| `AO`     | 10 kΩ upper divider resistor |
| `DO`     | disconnected                 |

### MQ-4 voltage divider

| connection  | exact wiring                                |
| ----------- | ------------------------------------------- |
| resistor 1  | MQ-4 `AO` → **10 kΩ** → divider junction    |
| ESP32 input | divider junction → ESP32 `VP`               |
| resistor 2  | divider junction → **15 kΩ** → `COMMON_GND` |

`VP` firmware mein `GPIO36` hai.

divider output:

[
V_{\text{ESP32}} =
V_{\text{MQ4}}
\times
\frac{15,000}{10,000 + 15,000}
]

agar MQ-4 output 5 V hua:

[
5.0 \times \frac{15}{25} = 3.0\text{ V}
]

isliye ESP32 input safe range mein rahega.

**MQ-4 `AO` ko `VP` se directly connect mat karna.**

MQ-4 ko warm-up chahiye, aur bina proper calibration ke reading ko accurate methane ppm mat samajhna.

---

## 12. HC-SR04 ultrasonic sensor

| HC-SR04 pin | connect to                   |
| ----------- | ---------------------------- |
| `VCC`       | `LOGIC_5V`                   |
| `GND`       | `COMMON_GND`                 |
| `TRIG`      | ESP32 `D19`                  |
| `ECHO`      | 10 kΩ upper divider resistor |

### HC-SR04 `ECHO` divider

| connection  | exact wiring                                  |
| ----------- | --------------------------------------------- |
| resistor 1  | HC-SR04 `ECHO` → **10 kΩ** → divider junction |
| ESP32 input | divider junction → ESP32 `D18`                |
| resistor 2  | divider junction → **15 kΩ** → `COMMON_GND`   |

firmware:

| signal | GPIO |
| ------ | ---: |
| `TRIG` |   19 |
| `ECHO` |   18 |

`ECHO` direct connect karoge to approximately 5 V ESP32 GPIO par ja sakta hai.

HC-SR04 chassis ke front mein fixed position par mount karo.

---

## 13. divider junction physically kya hota hai

har divider junction mein **teen connections ek hi electrical point** par milenge.

### MQ-4 junction

| junction par connected items                        |
| --------------------------------------------------- |
| MQ-4 `AO` se aane wale 10 kΩ resistor ka second end |
| ESP32 `VP` ki signal wire                           |
| `GND` jaane wale 15 kΩ resistor ka first end        |

### HC-SR04 junction

| junction par connected items                     |
| ------------------------------------------------ |
| `ECHO` se aane wale 10 kΩ resistor ka second end |
| ESP32 `D18` ki signal wire                       |
| `GND` jaane wale 15 kΩ resistor ka first end     |

breadboard par ye teen legs **same connected row** mein lagenge. soldered build mein teenon ek common solder point par join honge.

resistors non-polarized hote hain: unki direction matter nahi karti.

---

## 14. MPU6050 GY-521

| MPU6050 pin | connect to                     |
| ----------- | ------------------------------ |
| `VCC`       | ESP32 `3V3`                    |
| `GND`       | `COMMON_GND`                   |
| `SDA`       | ESP32 `D21`                    |
| `SCL`       | ESP32 `D22`                    |
| `AD0`       | `GND`, or existing default low |
| `INT`       | disconnected                   |
| `XDA`       | disconnected                   |
| `XCL`       | disconnected                   |

expected I²C address:

```text
0x68
```

agar tumhare particular GY-521 board ka regulator 3.3 V input par reliable nahi hai, module ko 5 V par tabhi power karna jab multimeter se confirm ho ki uske `SDA` aur `SCL` idle voltage **3.3 V se upar nahi** ja rahe.

---

## 15. VL53L0X distance sensor

| VL53L0X pin                 | connect to                                                             |
| --------------------------- | ---------------------------------------------------------------------- |
| `VIN` / `VCC`               | ESP32 `3V3`                                                            |
| `GND`                       | `COMMON_GND`                                                           |
| `SDA`                       | ESP32 `D21`                                                            |
| `SCL`                       | ESP32 `D22`                                                            |
| `XSHUT`, if present         | default state; disconnected unless module documentation says otherwise |
| `GPIO1` / `INT`, if present | disconnected                                                           |

expected Arduino I²C address:

```text
0x29
```

datasheets kabhi `0x52` dikhate hain because woh shifted address representation hota hai; Arduino I²C scanner mein normally **`0x29`** dikhega. [ST VL53L0X datasheet](https://www.st.com/resource/en/datasheet/vl53l0x.pdf)

VL53L0X ko SG90 servo horn par mount karo.

---

## 16. MPU6050 aur VL53L0X shared I²C bus

| ESP32 pin | first connection | second connection |
| --------- | ---------------- | ----------------- |
| `D21`     | MPU6050 `SDA`    | VL53L0X `SDA`     |
| `D22`     | MPU6050 `SCL`    | VL53L0X `SCL`     |
| `3V3`     | MPU6050 `VCC`    | VL53L0X `VIN`     |
| `GND`     | MPU6050 `GND`    | VL53L0X `GND`     |

dono sensors same wires share kar sakte hain because addresses different hain:

| device  | I²C address |
| ------- | ----------- |
| MPU6050 | `0x68`      |
| VL53L0X | `0x29`      |

normally breakout boards par I²C pull-ups already hote hain. agar bilkul pull-ups nahi hain, add:

| resistor | connect between         |
| -------- | ----------------------- |
| 4.7 kΩ   | `D21` / `SDA` and `3V3` |
| 4.7 kΩ   | `D22` / `SCL` and `3V3` |

ye extra resistors tabhi required hain jab modules mein onboard pull-ups absent hon.

---

## 17. left LM393 wheel encoder

| left encoder pin | connect to   |
| ---------------- | ------------ |
| `VCC`            | ESP32 `3V3`  |
| `GND`            | `COMMON_GND` |
| `DO` / `OUT`     | ESP32 `D34`  |
| `AO`, if present | disconnected |

external pull-up:

| resistor | first end            | second end  |
| -------- | -------------------- | ----------- |
| 10 kΩ    | `D34` / encoder `DO` | ESP32 `3V3` |

firmware pin: `GPIO34`.

---

## 18. right LM393 wheel encoder

| right encoder pin | connect to   |
| ----------------- | ------------ |
| `VCC`             | ESP32 `3V3`  |
| `GND`             | `COMMON_GND` |
| `DO` / `OUT`      | ESP32 `D35`  |
| `AO`, if present  | disconnected |

external pull-up:

| resistor | first end            | second end  |
| -------- | -------------------- | ----------- |
| 10 kΩ    | `D35` / encoder `DO` | ESP32 `3V3` |

firmware pin: `GPIO35`.

LM393 output open-collector hota hai, isliye valid HIGH signal ke liye pull-up required hota hai. kuch modules mein pull-up already installed hota hai, lekin `D34` / `D35` mein ESP32 ka internal pull-up nahi hota. [Texas Instruments LM393 application guide](https://www.ti.com/lit/pdf/snoaa35)

**encoder modules ko 5 V se power mat karo** unless tumne output level separately safe banaya ho. 3.3 V supply se `DO` output ESP32-compatible rahega.

encoder disc aur optical slot properly align karo. module ka adjustment potentiometer rotate karke indicator LED ko slots ke saath toggle karwao.

---

## 19. water-level sensor

| water sensor pin | connect to   |
| ---------------- | ------------ |
| `+` / `VCC`      | ESP32 `3V3`  |
| `−` / `GND`      | `COMMON_GND` |
| `S` / `SIG`      | ESP32 `VN`   |

`VN` = **`GPIO39`**.

| board label | code             |
| ----------- | ---------------- |
| `VN`        | `analogRead(39)` |

sensor ko 5 V supply doge aur signal directly `VN` par connect karoge to ESP32 overvoltage risk hoga. is design mein supply **3.3 V** hi rakho.

---

## 20. SG90 servo

| SG90 wire color         | connect to   |
| ----------------------- | ------------ |
| brown / black           | `COMMON_GND` |
| red                     | `LOGIC_5V`   |
| orange / yellow / white | ESP32 `D13`  |

firmware pin: `GPIO13`.

servo ka 5 V connection buck converter output se aayega. servo ko ESP32 `3V3` pin se power mat karo.

servo ko physically VL53L0X sensor ke saath mount karo.

---

## 21. rover red LED

| LED connection               | destination                 |
| ---------------------------- | --------------------------- |
| ESP32 `D26`                  | 220 Ω resistor ka first end |
| 220 Ω resistor ka second end | red LED long leg / anode    |
| red LED short leg / cathode  | `COMMON_GND`                |

firmware pin: `GPIO26`.

---

## 22. rover green LED

| LED connection                | destination                 |
| ----------------------------- | --------------------------- |
| ESP32 `D27`                   | 220 Ω resistor ka first end |
| 220 Ω resistor ka second end  | green LED long leg / anode  |
| green LED short leg / cathode | `COMMON_GND`                |

firmware pin: `GPIO27`.

**har LED ka apna resistor hona chahiye.**

---

## 23. active buzzer

yahan do possible hardware variants hain.

### case A: three-pin active-buzzer module

agar module par `VCC`, `GND`, aur `S` / `IN` printed hai:

| buzzer module pin | connect to                                       |
| ----------------- | ------------------------------------------------ |
| `VCC`             | module rating ke according `3V3` or `LOGIC_5V`   |
| `GND`             | `COMMON_GND`                                     |
| `S` / `IN`        | ESP32 `D4`, only if module accepts 3.3 V control |

firmware pin: `GPIO4`.

agar signal pin 5 V par pull-up hota hai, usse ESP32 par directly connect nahi karna.

### case B: two-pin 5 V active buzzer

safe wiring ke liye additional components:

| extra component                               |                                 quantity |
| --------------------------------------------- | ---------------------------------------: |
| NPN transistor: `2N2222`, `BC547`, or `S8050` |                                        1 |
| 1 kΩ resistor                                 |                                        1 |
| diode `1N4148` / `1N4007`                     | 1 if buzzer is inductive/electromagnetic |

connections:

| component terminal                 | connect to                        |
| ---------------------------------- | --------------------------------- |
| buzzer `+`                         | `LOGIC_5V`                        |
| buzzer `−`                         | transistor collector              |
| transistor emitter                 | `COMMON_GND`                      |
| transistor base                    | 1 kΩ resistor                     |
| 1 kΩ resistor other end            | ESP32 `D4`                        |
| diode striped end, if required     | buzzer `+` / `LOGIC_5V`           |
| diode non-striped end, if required | buzzer `−` / transistor collector |

**correction:** earlier jo `GPIO4 → 220 Ω → active buzzer` bataya tha, woh every 5 V active buzzer ke liye proper universal solution nahi hai. two-pin 5 V active buzzer ke liye transistor driver safer arrangement hai.

transistor ki physical leg order exact model par depend karegi; `BC547`, `2N2222`, aur `S8050` ka pinout interchangeably assume mat karna.

---

## 24. complete capacitor placement

tumhare existing capacitors:

| capacitor           | quantity |
| ------------------- | -------: |
| 470 µF electrolytic |        2 |
| 100 nF ceramic      |        3 |

### capacitor 1: logic 5 V rail

| capacitor terminal  | connect to   |
| ------------------- | ------------ |
| 470 µF positive leg | `LOGIC_5V`   |
| 470 µF negative leg | `COMMON_GND` |

isko buck output aur servo power connection ke near lagao.

### capacitor 2: motor rail

| capacitor terminal  | connect to                     |
| ------------------- | ------------------------------ |
| 470 µF positive leg | `MOTOR_POS` / both TB6612 `VM` |
| 470 µF negative leg | `COMMON_GND`                   |

isko motor-driver power input ke near lagao.

### capacitor 3: left driver logic

| capacitor terminal    | connect to        |
| --------------------- | ----------------- |
| 100 nF ceramic side 1 | left TB6612 `VCC` |
| 100 nF ceramic side 2 | left TB6612 `GND` |

### capacitor 4: right driver logic

| capacitor terminal    | connect to         |
| --------------------- | ------------------ |
| 100 nF ceramic side 1 | right TB6612 `VCC` |
| 100 nF ceramic side 2 | right TB6612 `GND` |

### capacitor 5: ESP32 3.3 V rail

| capacitor terminal    | connect to  |
| --------------------- | ----------- |
| 100 nF ceramic side 1 | ESP32 `3V3` |
| 100 nF ceramic side 2 | ESP32 `GND` |

### identifying capacitor polarity

| component           | polarity                              |
| ------------------- | ------------------------------------- |
| 470 µF electrolytic | polarized; striped side is negative   |
| 100 nF ceramic      | non-polarized; either direction works |

100 nF ceramic capacitor par often:

```text
104
```

printed hota hai.

470 µF capacitor ka voltage rating ideally **16 V ya 25 V** ho.

---

## 25. resistor inventory and exact placement

| resistor         |                    quantity | first connection                          | second connection                         |
| ---------------- | --------------------------: | ----------------------------------------- | ----------------------------------------- |
| 10 kΩ #1         |                           1 | MQ-4 `AO`                                 | MQ-4 divider junction / ESP32 `VP`        |
| 15 kΩ #1         |                           1 | MQ-4 divider junction / ESP32 `VP`        | `COMMON_GND`                              |
| 10 kΩ #2         |                           1 | HC-SR04 `ECHO`                            | ultrasonic divider junction / ESP32 `D18` |
| 15 kΩ #2         |                           1 | ultrasonic divider junction / ESP32 `D18` | `COMMON_GND`                              |
| 10 kΩ #3         |                           1 | left encoder output / `D34`               | `3V3`                                     |
| 10 kΩ #4         |                           1 | right encoder output / `D35`              | `3V3`                                     |
| 220 Ω #1         |                           1 | ESP32 `D26`                               | red rover LED long leg                    |
| 220 Ω #2         |                           1 | ESP32 `D27`                               | green rover LED long leg                  |
| additional 10 kΩ |         only for bare DHT22 | DHT22 `DATA`                              | `3V3`                                     |
| additional 1 kΩ  | only for two-pin 5 V buzzer | ESP32 `D4`                                | NPN transistor base                       |

common four-band resistor colors:

| resistor | typical color bands        |
| -------- | -------------------------- |
| 220 Ω    | red, red, brown, gold      |
| 1 kΩ     | brown, black, red, gold    |
| 10 kΩ    | brown, black, orange, gold |
| 15 kΩ    | brown, green, orange, gold |

gold band usually tolerance band hoti hai.

agar colors unclear hon, multimeter resistance mode mein verify karo.

---

## 26. rover complete component-to-pin summary

| component     | power                           | ground        | signal connections     | extra components                        |
| ------------- | ------------------------------- | ------------- | ---------------------- | --------------------------------------- |
| ESP32         | `VIN` ← regulated 5 V           | common ground | —                      | 100 nF across `3V3` / `GND`             |
| DHT22         | `3V3`                           | common ground | `D23`                  | 10 kΩ only if bare sensor               |
| MQ-4          | 5 V                             | common ground | `VP` / `GPIO36`        | 10 kΩ + 15 kΩ divider                   |
| HC-SR04       | 5 V                             | common ground | `TRIG=D19`, `ECHO=D18` | 10 kΩ + 15 kΩ ECHO divider              |
| MPU6050       | `3V3`                           | common ground | `SDA=D21`, `SCL=D22`   | normally none                           |
| VL53L0X       | `3V3`                           | common ground | `SDA=D21`, `SCL=D22`   | normally none                           |
| left encoder  | `3V3`                           | common ground | `D34`                  | 10 kΩ pull-up                           |
| right encoder | `3V3`                           | common ground | `D35`                  | 10 kΩ pull-up                           |
| water sensor  | `3V3`                           | common ground | `VN` / `GPIO39`        | none                                    |
| SG90          | 5 V                             | common ground | `D13`                  | 470 µF on 5 V rail                      |
| red LED       | GPIO-powered                    | common ground | `D26`                  | 220 Ω                                   |
| green LED     | GPIO-powered                    | common ground | `D27`                  | 220 Ω                                   |
| active buzzer | 3.3 V / 5 V depending on module | common ground | `D4`                   | driver transistor if two-pin 5 V buzzer |
| left TB6612   | `VCC=3V3`; `VM=motor battery`   | common ground | `D25`, `D14`, `RX2`    | 100 nF across `VCC` / `GND`             |
| right TB6612  | `VCC=3V3`; `VM=motor battery`   | common ground | `TX2`, `D33`, `D32`    | 100 nF across `VCC` / `GND`             |

---

## 27. gateway ESP32 connections

gateway separate board hai. laptop USB usse power aur serial communication provide karega.

| gateway component           | connection                     |
| --------------------------- | ------------------------------ |
| gateway ESP32 USB           | laptop USB                     |
| LCD `SDA`                   | gateway `GPIO21`               |
| LCD `SCL`                   | gateway `GPIO22`               |
| red LED                     | gateway `GPIO25` through 220 Ω |
| yellow LED                  | gateway `GPIO26` through 220 Ω |
| green LED                   | gateway `GPIO27` through 220 Ω |
| LCD ground and LED cathodes | gateway `GND`                  |

gateway aur rover ke GPIO numbers overlap kar sakte hain because ye **alag ESP32 boards** hain.

---

## 28. gateway LCD: safest wiring

ye wiring **four-pin I²C LCD backpack** ke liye hai.

### option 1: LCD reliably works at 3.3 V

| LCD pin | gateway ESP32 |
| ------- | ------------- |
| `VCC`   | `3V3`         |
| `GND`   | `GND`         |
| `SDA`   | `GPIO21`      |
| `SCL`   | `GPIO22`      |

ye electrically simple aur safe hai, but kuch 16×2 LCD modules 3.3 V par low contrast ya unreliable behavior dikha sakte hain.

backpack ka contrast potentiometer adjust karna pad sakta hai.

### option 2: LCD requires 5 V

bidirectional I²C level shifter use karo:

| level-shifter pin | connect to           |
| ----------------- | -------------------- |
| `LV`              | gateway `3V3`        |
| `HV`              | gateway `5V` / `VIN` |
| `GND`             | gateway `GND`        |
| `LV1`             | gateway `GPIO21`     |
| `HV1`             | LCD `SDA`            |
| `LV2`             | gateway `GPIO22`     |
| `HV2`             | LCD `SCL`            |

LCD power:

| LCD pin | connect to    |
| ------- | ------------- |
| `VCC`   | gateway `5V`  |
| `GND`   | gateway `GND` |

LCD I²C address usually:

```text
0x27
```

occasionally:

```text
0x3F
```

agar backpack `SDA` / `SCL` ko 5 V par pull-up karta hai, ESP32 ke saath direct connection unsafe hai.

---

## 29. gateway LEDs

| LED    | gateway GPIO | exact wiring                                       |
| ------ | -----------: | -------------------------------------------------- |
| red    |         `25` | `GPIO25` → 220 Ω → LED long leg; short leg → `GND` |
| yellow |         `26` | `GPIO26` → 220 Ω → LED long leg; short leg → `GND` |
| green  |         `27` | `GPIO27` → 220 Ω → LED long leg; short leg → `GND` |

### total LED resistor requirement

| LEDs                         | 220 Ω resistors |
| ---------------------------- | --------------: |
| rover red + green            |               2 |
| gateway red + yellow + green |               3 |
| total                        |           **5** |

agar tumhare paas total sirf **3×220 Ω** resistors hain, five LEDs simultaneously correct wiring ke liye **2 additional 220 Ω resistors** chahiye.

---

## 30. complete firmware pin definitions

```cpp
#pragma once

#include <Arduino.h>

namespace RoverPins {

// Environmental sensors
constexpr uint8_t DHT22 = 23;       // Board: D23
constexpr uint8_t MQ4 = 36;        // Board: VP
constexpr uint8_t WATER = 39;      // Board: VN

// Fixed front ultrasonic sensor
constexpr uint8_t ULTRASONIC_TRIG = 19;  // Board: D19
constexpr uint8_t ULTRASONIC_ECHO = 18;  // Board: D18

// Shared I2C bus
constexpr uint8_t I2C_SDA = 21;    // Board: D21
constexpr uint8_t I2C_SCL = 22;    // Board: D22

// Wheel encoders
constexpr uint8_t ENCODER_LEFT = 34;   // Board: D34
constexpr uint8_t ENCODER_RIGHT = 35;  // Board: D35

// Scanning servo
constexpr uint8_t SERVO = 13;      // Board: D13

// Rover indicators
constexpr uint8_t BUZZER = 4;      // Board: D4
constexpr uint8_t LED_RED = 26;    // Board: D26
constexpr uint8_t LED_GREEN = 27;  // Board: D27

// Left TB6612: front and rear channels share these signals
constexpr uint8_t MOTOR_LEFT_PWM = 25;  // Board: D25
constexpr uint8_t MOTOR_LEFT_IN1 = 14;  // Board: D14
constexpr uint8_t MOTOR_LEFT_IN2 = 16;  // Board: RX2

// Right TB6612: front and rear channels share these signals
constexpr uint8_t MOTOR_RIGHT_PWM = 17;  // Board: TX2
constexpr uint8_t MOTOR_RIGHT_IN1 = 33;  // Board: D33
constexpr uint8_t MOTOR_RIGHT_IN2 = 32;  // Board: D32

// Both driver STBY pins are wired directly to 3V3.
// Therefore there is intentionally no STBY GPIO constant.

constexpr uint8_t MPU6050_ADDRESS = 0x68;
constexpr uint8_t VL53L0X_ADDRESS = 0x29;

}  // namespace RoverPins

namespace GatewayPins {

constexpr uint8_t LCD_SDA = 21;
constexpr uint8_t LCD_SCL = 22;

constexpr uint8_t LED_RED = 25;
constexpr uint8_t LED_YELLOW = 26;
constexpr uint8_t LED_GREEN = 27;

constexpr uint8_t LCD_ADDRESS = 0x27;

}  // namespace GatewayPins
```

important initialization:

```cpp
#include <Wire.h>

void setup() {
  Wire.begin(RoverPins::I2C_SDA, RoverPins::I2C_SCL);

  // GPIO34 and GPIO35 have no internal pull-ups.
  // Use the external 10k resistors described above.
  pinMode(RoverPins::ENCODER_LEFT, INPUT);
  pinMode(RoverPins::ENCODER_RIGHT, INPUT);

  // Configure analog channels for approximately 0–3.1 V.
  analogSetPinAttenuation(RoverPins::MQ4, ADC_11db);
  analogSetPinAttenuation(RoverPins::WATER, ADC_11db);

  pinMode(RoverPins::ULTRASONIC_TRIG, OUTPUT);
  pinMode(RoverPins::ULTRASONIC_ECHO, INPUT);

  pinMode(RoverPins::LED_RED, OUTPUT);
  pinMode(RoverPins::LED_GREEN, OUTPUT);
  pinMode(RoverPins::BUZZER, OUTPUT);

  pinMode(RoverPins::MOTOR_LEFT_IN1, OUTPUT);
  pinMode(RoverPins::MOTOR_LEFT_IN2, OUTPUT);
  pinMode(RoverPins::MOTOR_RIGHT_IN1, OUTPUT);
  pinMode(RoverPins::MOTOR_RIGHT_IN2, OUTPUT);

  digitalWrite(RoverPins::MOTOR_LEFT_IN1, LOW);
  digitalWrite(RoverPins::MOTOR_LEFT_IN2, LOW);
  digitalWrite(RoverPins::MOTOR_RIGHT_IN1, LOW);
  digitalWrite(RoverPins::MOTOR_RIGHT_IN2, LOW);
}
```

`ADC_11db` classic ESP32 par approximately **150 mV–3100 mV** measurement range support karta hai, jo MQ-4 divider ke approximately 3.0 V maximum ke saath suitable hai. [Arduino-ESP32 ADC documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html)

---

## 31. components that may still be missing

| component                                | kab required hai                                                            |
| ---------------------------------------- | --------------------------------------------------------------------------- |
| 2×15 male header pins                    | tumhare photographed ESP32 par jumper connections ke liye                   |
| 2S BMS                                   | 2×18650 series battery safely use karne ke liye                             |
| 8.4 V lithium charger                    | 2S battery charge karne ke liye                                             |
| buck converter, approximately 3 A        | 2S battery ko stable 5 V mein convert karne ke liye                         |
| additional 2×220 Ω resistors             | agar total five LEDs hain but currently only three resistors available hain |
| NPN transistor + 1 kΩ resistor           | agar buzzer two-pin 5 V active buzzer hai                                   |
| bidirectional I²C level shifter          | agar gateway LCD 5 V par hi reliably kaam karta hai                         |
| optional 2×4.7 kΩ resistors              | agar MPU6050/VL53L0X modules mein onboard I²C pull-ups absent hain          |
| switches                                 | optional logic-power and motor-power isolation                              |
| soldering supplies / terminal connectors | motor-current wiring and secure assembly                                    |

---

## 32. first-power-on checklist

1. **header pins solder karo** aur inspect karo ki adjacent pins short na hon.
2. BMS battery connections verify karo: `B-`, middle connection, `B+`.
3. buck output ko multimeter se **5.0 V** set karo.
4. motor battery positive aur logic 5 V positive separate verify karo.
5. motor battery negative aur ESP32 ground common verify karo.
6. ESP32 `VIN` par approximately **5 V** confirm karo.
7. ESP32 `3V3` pin par approximately **3.3 V** confirm karo.
8. dono TB6612 `VCC` par approximately **3.3 V** confirm karo.
9. dono TB6612 `STBY` par approximately **3.3 V** confirm karo.
10. dono TB6612 `VM` par approximately **4.8–5.6 V** confirm karo.
11. MQ-4 divider junction ko `VP` se connect karo; output **3.3 V se zyada** nahi hona chahiye.
12. HC-SR04 `ECHO` direct GPIO se connected na ho; divider installed hona chahiye.
13. encoders `D34` / `D35` par 10 kΩ pull-ups verify karo.
14. water sensor power **3.3 V** verify karo.
15. I²C scanner mein MPU6050 `0x68` aur VL53L0X `0x29` check karo.
16. wheels ko surface se utha kar one-by-one motor direction test karo.
17. servo test karo.
18. gateway LCD aur LEDs test karo.
19. ESP-NOW link establish karo.
20. gateway disconnect karke confirm karo ki rover approximately **500 ms** ke andar motors stop/brake karta hai.
