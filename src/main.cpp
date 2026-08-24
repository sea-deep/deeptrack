#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>

#define USE_ESP_NOW 0

#include <DHT.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_VL53L0X.h>
#include <ESP32Servo.h>
#include "telemetry_packet.h"

// --- Pin Definitions (Original Preserved) ---
#define DHT_PIN            23
#define GAS_PIN            34
#define TRIG_PIN           19
#define ECHO_PIN           18
#define ENC_LEFT_PIN       32
#define ENC_RIGHT_PIN      35
#define WATER_PIN          36
#define SERVO_PIN          13
#define BUZZER_PIN         4
#define LED_RED_PIN        26
#define LED_GREEN_PIN      27
#define I2C_SDA            21
#define I2C_SCL            22

// Motor Pins (TB6612FNG)
#define L_PWM              25
#define L_DIR1             16
#define L_DIR2             17
#define R_PWM              14
#define R_DIR1             33
#define R_DIR2             2
#define STBY_PIN           5

// --- Constants & Config ---
#define DHT_TYPE           DHT22
#define OBSTACLE_STOP_CM   15
#define L_PWM_CH           0
#define R_PWM_CH           1
#define PWM_FREQ           5000
#define PWM_RES            8

// --- Globals ---
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_MPU6050 mpu;
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
Servo scanServo;

uint8_t gatewayAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 

// State
TelemetryPacket currentTelemetry;
uint32_t lastTelemetryTime = 0;
bool emergencyStopped = false;

// Encoders
volatile uint32_t leftPulses = 0;
volatile uint32_t rightPulses = 0;

void IRAM_ATTR onLeftPulse() { leftPulses++; }
void IRAM_ATTR onRightPulse() { rightPulses++; }

// Scan State
int currentAngle = 90;
int scanDirection = 10;
uint32_t lastScanTime = 0;
uint8_t scanSeq = 0;

// Motor state (target)
int target_l = 0;
int target_r = 0;

// --- Function Prototypes ---
void setMotors(int left, int right);
void shortBrake();

// --- ESP-NOW Callback ---
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(ControlPacket)) {
        ControlPacket *pkt = (ControlPacket*)incomingData;
        if (pkt->type == 0) { 
            target_l = pkt->motor_l;
            target_r = pkt->motor_r;
            emergencyStopped = false; // Reset emergency on new command
        }
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL);

    // Motor Pins
    pinMode(L_DIR1, OUTPUT);
    pinMode(L_DIR2, OUTPUT);
    pinMode(R_DIR1, OUTPUT);
    pinMode(R_DIR2, OUTPUT);
    pinMode(STBY_PIN, OUTPUT);
    
    ledcSetup(L_PWM_CH, PWM_FREQ, PWM_RES);
    ledcSetup(R_PWM_CH, PWM_FREQ, PWM_RES);
    ledcAttachPin(L_PWM, L_PWM_CH);
    ledcAttachPin(R_PWM, R_PWM_CH);
    
    digitalWrite(STBY_PIN, HIGH); // Enable driver

    // LED & Buzzer
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);

    // Ultrasonic
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // Sensors
    pinMode(GAS_PIN, INPUT);
    pinMode(WATER_PIN, INPUT);

    // Encoders
    pinMode(ENC_LEFT_PIN, INPUT_PULLUP);
    pinMode(ENC_RIGHT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENC_LEFT_PIN), onLeftPulse, RISING);
    attachInterrupt(digitalPinToInterrupt(ENC_RIGHT_PIN), onRightPulse, RISING);

    dht.begin();
    scanServo.attach(SERVO_PIN, 500, 2400);
    
    if (!mpu.begin()) Serial.println("Failed to find MPU6050");
    if (!lox.begin(0x29, true)) {
        Serial.println("Failed to boot VL53L0X");
    }

    WiFi.mode(WIFI_STA);
#if USE_ESP_NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(onDataRecv);
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, gatewayAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
#endif
}

void shortBrake() {
    digitalWrite(STBY_PIN, HIGH);
    digitalWrite(L_DIR1, HIGH);
    digitalWrite(L_DIR2, HIGH);
    digitalWrite(R_DIR1, HIGH);
    digitalWrite(R_DIR2, HIGH);
    ledcWrite(L_PWM_CH, 255);
    ledcWrite(R_PWM_CH, 255);
}

void setMotors(int left, int right) {
    if (emergencyStopped) return; 
    
    digitalWrite(STBY_PIN, HIGH);

    if (left > 0) {
        digitalWrite(L_DIR1, HIGH);
        digitalWrite(L_DIR2, LOW);
    } else if (left < 0) {
        digitalWrite(L_DIR1, LOW);
        digitalWrite(L_DIR2, HIGH);
    } else {
        digitalWrite(L_DIR1, HIGH);
        digitalWrite(L_DIR2, HIGH);
    }
    ledcWrite(L_PWM_CH, abs(left));

    if (right > 0) {
        digitalWrite(R_DIR1, HIGH);
        digitalWrite(R_DIR2, LOW);
    } else if (right < 0) {
        digitalWrite(R_DIR1, LOW);
        digitalWrite(R_DIR2, HIGH);
    } else {
        digitalWrite(R_DIR1, HIGH);
        digitalWrite(R_DIR2, HIGH);
    }
    ledcWrite(R_PWM_CH, abs(right));
}

float readUltrasonic() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
    if (duration == 0) return 999.0;
    return (duration * 0.0343) / 2.0;
}

void loop() {
    uint32_t now = millis();

    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        if (cmd.startsWith("L:")) {
            int spaceIdx = cmd.indexOf(' ');
            if (spaceIdx > -1) {
                target_l = cmd.substring(2, spaceIdx).toInt();
                target_r = cmd.substring(cmd.indexOf("R:") + 2).toInt();
                emergencyStopped = false;
            }
        }
    }


    // 1. Ultrasonic Obstacle Check (Fixed Forward)
    float frontDist = readUltrasonic();
    if (frontDist < OBSTACLE_STOP_CM && frontDist > 0) {
        if (!emergencyStopped && (target_l > 0 || target_r > 0)) {
            Serial.println("OBSTACLE - EMERGENCY STOP");
            shortBrake();
            emergencyStopped = true;
        }
    }

    // Apply motor state
    if (!emergencyStopped) {
        setMotors(target_l, target_r);
    }

    // 2. Servo Sweep & Scan (VL53L0X)
    if (now - lastScanTime > 33) {
        lastScanTime = now;
        scanServo.write(currentAngle);
        
        VL53L0X_RangingMeasurementData_t measure;
        lox.rangingTest(&measure, false);
        
        ScanPacket spkt;
        spkt.type = 1;
        spkt.seq = scanSeq++;
        spkt.angle_deg = currentAngle;
        spkt.timestamp_ms = now;
        
        if (measure.RangeStatus != 4) { 
            spkt.distance_mm = measure.RangeMilliMeter;
            spkt.valid = 1;
        } else {
            spkt.distance_mm = 800;
            spkt.valid = 0;
        }
#if USE_ESP_NOW
        
        
        esp_now_send(gatewayAddress, (uint8_t *) &spkt, sizeof(ScanPacket));
#endif
        
        char json[128];
        snprintf(json, sizeof(json),
            "{\"type\":\"scan\",\"seq\":%d,\"angle_deg\":%d,\"distance_mm\":%d,\"valid\":%s,\"timestamp_ms\":%u}",
            spkt.seq, spkt.angle_deg, spkt.distance_mm, spkt.valid ? "true" : "false", spkt.timestamp_ms
        );
        Serial.println(json);

        
        currentAngle += scanDirection;
        if (currentAngle >= 150 || currentAngle <= 30) {
            scanDirection = -scanDirection;
        }
    }

    // 3. Environmental Telemetry
    if (now - lastTelemetryTime > 1000) {
        lastTelemetryTime = now;

        currentTelemetry.temperature = dht.readTemperature();
        currentTelemetry.humidity = dht.readHumidity();
        currentTelemetry.gasRaw = analogRead(GAS_PIN);
        currentTelemetry.waterRaw = analogRead(WATER_PIN);
        
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        currentTelemetry.ax = a.acceleration.x;
        currentTelemetry.ay = a.acceleration.y;
        currentTelemetry.az = a.acceleration.z;
        currentTelemetry.gx = g.gyro.x;
        currentTelemetry.gy = g.gyro.y;
        currentTelemetry.gz = g.gyro.z;
        
        currentTelemetry.dangerState = emergencyStopped ? 1 : 0;
        
        digitalWrite(LED_RED_PIN, emergencyStopped ? HIGH : LOW);
        digitalWrite(LED_GREEN_PIN, emergencyStopped ? LOW : HIGH);
        if (emergencyStopped) {
            tone(BUZZER_PIN, 1000, 100);
        } else {
            noTone(BUZZER_PIN);
        }
#if USE_ESP_NOW

        
        esp_now_send(gatewayAddress, (uint8_t *) &currentTelemetry, sizeof(TelemetryPacket));
#endif
        
        char json[256];
        snprintf(json, sizeof(json),
            "{\"t\":%.1f,\"h\":%.1f,\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f,\"gas\":%d,\"water\":%d,\"danger\":%d}",
            currentTelemetry.temperature, currentTelemetry.humidity,
            currentTelemetry.ax, currentTelemetry.ay, currentTelemetry.az,
            currentTelemetry.gx, currentTelemetry.gy, currentTelemetry.gz,
            currentTelemetry.gasRaw, currentTelemetry.waterRaw,
            currentTelemetry.dangerState
        );
        Serial.println(json);

    }
}
