/*
  DEEPTRACK GATEWAY — ESP-NOW Receiver + LCD Console + Hot-Pluggable MQTT & Serial Relay

  This ESP32 sits on a desk, acting as the base station console.
  It receives telemetry from the Rover ESP32 over ESP-NOW (real hardware)
  or over a Serial bridge (Wokwi simulation via socat + rfc2217).
  If neither source provides data within 5 seconds, a built-in demo
  data generator activates so the LCD, LEDs, and Dashboard can be tested.

  Telemetry Relaying (Hot-Pluggable Dual-Output):
    1. USB Serial        → 115200 baud JSON stream to laptop (Web Serial API)
    2. MQTT Stream       → Publishes JSON to broker.hivemq.com (non-blocking)

  Components:
    16×2 I2C LCD (0x27)  → SDA 21 / SCL 22 (Auto-cycles every 3s between ENV/NAV/STATUS)
    Red LED              → GPIO 26 (DANGER indicator)
    Green LED            → GPIO 27 (NORMAL / LINK OK)
    Yellow LED           → GPIO 25 (Heartbeat blink on packet reception)
*/

#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include "telemetry_packet.h"

// ---------- WiFi & MQTT Config (Hot-pluggable / non-blocking) ----------
const char* WIFI_SSID   = "Wokwi-GUEST";
const char* WIFI_PASS   = "";
const char* MQTT_SERVER = "broker.hivemq.com";
const int   MQTT_PORT   = 1883;
const char* MQTT_TOPIC  = "deeptrack/rover/telemetry";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastMqttAttempt = 0;
bool mqttWasConnected = false;

// ---------- Pin map ----------
#define LED_RED_PIN     26
#define LED_GREEN_PIN   27
#define LED_YELLOW_PIN  25

// ---------- LCD ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Custom characters (HD44780 can store 8 custom chars, 5×8 pixels each)
byte thermIcon[8] = {
  0b00100, 0b01010, 0b01010, 0b01110,
  0b01110, 0b11111, 0b11111, 0b01110
};
byte signalIcon[8] = {
  0b00001, 0b00001, 0b00101, 0b00101,
  0b10101, 0b10101, 0b10101, 0b00000
};
byte warnIcon[8] = {
  0b00000, 0b00100, 0b00100, 0b01010,
  0b01110, 0b11111, 0b11111, 0b00000
};

// ---------- State ----------
TelemetryPacket telem;              // latest telemetry data
volatile bool espNowReady = false;  // set by ISR-like callback
bool linkActive = false;
bool demoActive = false;

unsigned long lastDataTime   = 0;   // millis() of last received packet
unsigned long lastPageSwitch = 0;
unsigned long lastHeartbeatOn = 0;
unsigned long lastDemoTick   = 0;
uint32_t packetCount = 0;

int lcdPage = 0;
const int NUM_PAGES = 3;  // ENV, NAV, STATUS

// Demo mode synthetic state
float demoX = 0, demoY = 0, demoH = 0;
uint32_t demoTick = 0;

// ---------- ESP-NOW receive callback ----------
void onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
  if (len == sizeof(TelemetryPacket)) {
    memcpy(&telem, data, sizeof(TelemetryPacket));
    espNowReady = true;
  }
}

// ---------- LCD display ----------
void updateLCD() {
  lcd.clear();

  // DANGER page overrides everything
  if (telem.state == 3) {
    lcd.setCursor(0, 0);
    lcd.write(2); // warning icon
    lcd.print(" DANGER!!  ");
    lcd.write(2);
    lcd.setCursor(0, 1);
    const char* cause = (telem.dangerCause < 7) ? DANGER_NAMES[telem.dangerCause] : "???";
    char line2[17];
    snprintf(line2, sizeof(line2), ">>> %-7s <<<", cause);
    lcd.print(line2);
    return;
  }

  switch (lcdPage) {
    case 0: { // ENV — temperature, humidity, gas, water
      lcd.setCursor(0, 0);
      lcd.write(0); // thermometer icon
      char e1[17];
      snprintf(e1, sizeof(e1), "%.1fC H:%.0f%%", telem.tempC, telem.humidity);
      lcd.print(e1);
      lcd.setCursor(0, 1);
      char e2[17];
      snprintf(e2, sizeof(e2), "CH4:%04d Wt:%04d", (int)telem.gasRaw, (int)telem.waterRaw);
      lcd.print(e2);
      break;
    }
    case 1: { // NAV — position, heading, front distance
      lcd.setCursor(0, 0);
      char n1[17];
      snprintf(n1, sizeof(n1), "X:%-5.0f Y:%-5.0f", telem.x, telem.y);
      lcd.print(n1);
      lcd.setCursor(0, 1);
      char n2[17];
      snprintf(n2, sizeof(n2), "Hd:%-3.0f Ft:%-4.0f", telem.heading, telem.frontCm);
      lcd.print(n2);
      break;
    }
    case 2: { // STATUS — state, link, MQTT status, packet count
      lcd.setCursor(0, 0);
      const char* st = (telem.state < 4) ? STATE_NAMES[telem.state] : "???";
      char s1[17];
      snprintf(s1, sizeof(s1), "ST:%-6s M:%s", st, mqttClient.connected() ? "ON" : "--");
      lcd.print(s1);
      lcd.setCursor(0, 1);
      lcd.write(1); // signal icon
      char s2[17];
      snprintf(s2, sizeof(s2), "%s Pk:%lu",
               linkActive ? "OK " : "-- ",
               (unsigned long)packetCount);
      lcd.print(s2);
      break;
    }
  }
}

void showLinkLost() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(2); // warning
  lcd.print(" LINK LOST! ");
  lcd.write(2);
  lcd.setCursor(0, 1);
  lcd.print("No data >3s");
}

void showBootScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DEEPTRACK GTWRY");
  lcd.setCursor(0, 1);
  lcd.print("Waiting link...");
}

// ---------- LEDs ----------
void updateLEDs() {
  bool danger = (telem.state == 3);
  digitalWrite(LED_RED_PIN,   danger ? HIGH : LOW);
  digitalWrite(LED_GREEN_PIN, danger ? LOW  : HIGH);
}

void heartbeatBlink() {
  digitalWrite(LED_YELLOW_PIN, HIGH);
  lastHeartbeatOn = millis();
}

// ---------- Dual Relay: Serial + Hot-Pluggable MQTT ----------
void relayTelemetry() {
  const char* stName = (telem.state < 4) ? STATE_NAMES[telem.state] : "UNKNOWN";
  const char* dcName = (telem.dangerCause < 7) ? DANGER_NAMES[telem.dangerCause] : "NONE";

  char json[256];
  snprintf(json, sizeof(json),
    "{\"t\":%.1f,\"h\":%.1f,\"gas\":%d,\"dist\":%.1f,\"water\":%d,\"tilt\":%.1f,\"x\":%.1f,\"y\":%.1f,\"hd\":%.0f,\"st\":\"%s\",\"danger\":\"%s\"}",
    telem.tempC, telem.humidity,
    (int)telem.gasRaw, telem.frontCm,
    (int)telem.waterRaw, telem.tiltDeg,
    telem.x, telem.y, telem.heading,
    stName, dcName
  );

  // 1. Always output to USB Serial (for laptop Web Serial Dashboard)
  Serial.println(json);

  // 2. Publish to MQTT if connected (hot-pluggable)
  if (mqttClient.connected()) {
    mqttClient.publish(MQTT_TOPIC, json);
  }
}

// ---------- Parse JSON from Serial bridge ----------
bool parseJsonTelemetry(const String& line) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, line);
  if (err) return false;

  telem.tempC     = doc["t"] | doc["tempC"] | 0.0f;
  telem.humidity  = doc["h"] | doc["hum"]   | 0.0f;
  telem.gasRaw    = doc["gas"] | (int16_t)0;
  telem.frontCm   = doc["dist"] | doc["frontCm"] | 999.0f;
  telem.tofRaw    = doc["tof"] | doc["tofRaw"] | (int16_t)4095;
  telem.tiltDeg   = doc["tilt"] | doc["tiltDeg"] | 0.0f;
  telem.waterRaw  = doc["water"] | (int16_t)0;
  telem.encL      = doc["encL"] | (uint32_t)0;
  telem.encR      = doc["encR"] | (uint32_t)0;
  telem.x         = doc["x"] | 0.0f;
  telem.y         = doc["y"] | 0.0f;
  telem.heading   = doc["hd"] | doc["heading"] | 0.0f;

  // Parse state string → uint8_t
  const char* stStr = doc["st"] | doc["state"] | "NORMAL";
  telem.state = 0;
  for (int i = 0; i < 4; i++) {
    if (strcmp(stStr, STATE_NAMES[i]) == 0) { telem.state = i; break; }
  }

  const char* dcStr = doc["danger"] | doc["dangerCause"] | "NONE";
  telem.dangerCause = 0;
  for (int i = 0; i < 7; i++) {
    if (strcmp(dcStr, DANGER_NAMES[i]) == 0) { telem.dangerCause = i; break; }
  }

  return true;
}

// ---------- Demo data generator ----------
void generateDemoData() {
  demoTick++;
  bool danger  = (demoTick % 55 == 0);
  bool avoiding = (demoTick % 30 >= 25 && demoTick % 30 <= 29);

  telem.tempC    = 28.0f + sinf(demoTick / 10.0f) * 4.0f + (danger ? 22.0f : 0);
  telem.humidity = 60.0f + sinf(demoTick / 14.0f) * 10.0f;
  telem.gasRaw   = (int16_t)(400 + fabsf(sinf(demoTick / 8.0f)) * 300 + (danger ? 1500 : 0));
  telem.frontCm  = 20.0f + fabsf(sinf(demoTick / 5.0f)) * 60.0f;
  telem.tofRaw   = 4095;
  telem.tiltDeg  = 5.0f + fabsf(sinf(demoTick / 6.0f)) * 10.0f;
  telem.waterRaw = 300;
  telem.encL     = demoTick * 3;
  telem.encR     = demoTick * 3;

  if (avoiding && demoTick % 30 == 25) {
    demoH += ((demoTick % 2) ? 1.0f : -1.0f) * (40.0f + (float)(demoTick % 30));
  } else if (!danger) {
    demoH += (sinf(demoTick / 3.0f) - 0.5f) * 4.0f;
  }
  float step = danger ? -2.0f : (avoiding ? 1.0f : 4.0f);
  demoX += step * cosf(demoH * PI / 180.0f);
  demoY += step * sinf(demoH * PI / 180.0f);

  telem.x       = demoX;
  telem.y       = demoY;
  telem.heading = fmodf(demoH + 3600.0f, 360.0f);
  telem.state   = danger ? 3 : (avoiding ? 2 : (demoTick % 20 < 3 ? 1 : 0));
  telem.dangerCause = danger ? 1 : 0;  // GAS or NONE
}

// ---------- Process new data (from any source) ----------
void processNewData() {
  relayTelemetry();
  updateLEDs();
  updateLCD();
  heartbeatBlink();
  linkActive = true;
}

// ---------- Non-blocking WiFi & MQTT Handler ----------
void handleNetwork() {
  unsigned long now = millis();

  // If WiFi is disconnected, attempt connection in background without blocking
  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastMqttAttempt > 10000) {
      lastMqttAttempt = now;
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
    return;
  }

  // WiFi is connected -> handle MQTT
  if (!mqttClient.connected()) {
    if (now - lastMqttAttempt > 5000) {
      lastMqttAttempt = now;
      String clientId = "DEEPTRACK-GW-" + String(random(0xffff), HEX);
      if (mqttClient.connect(clientId.c_str())) {
        Serial.println("MQTT connected to " + String(MQTT_SERVER));
        mqttWasConnected = true;
      }
    }
  } else {
    mqttClient.loop();
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(200);

  // Pins
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);

  digitalWrite(LED_GREEN_PIN, HIGH);
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_YELLOW_PIN, LOW);

  // LCD init
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, thermIcon);
  lcd.createChar(1, signalIcon);
  lcd.createChar(2, warnIcon);
  showBootScreen();

  // WiFi in STA mode for both ESP-NOW & optional MQTT
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS); // Non-blocking connect attempt

  // ESP-NOW init
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
  } else {
    esp_now_register_recv_cb(onDataReceived);
    Serial.println("ESP-NOW receiver ready");
  }

  // MQTT setup
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setBufferSize(512);

  // Zero-init telemetry
  memset(&telem, 0, sizeof(telem));
  telem.frontCm = 999.0f;
  telem.tofRaw  = 4095;

  lastDataTime = millis();
  Serial.println("Gateway boot OK");
}

// ---------- Main loop ----------
void loop() {
  unsigned long now = millis();

  // Maintain WiFi & MQTT connectivity non-blockingly
  handleNetwork();

  // --- Source 1: ESP-NOW (real hardware) ---
  if (espNowReady) {
    espNowReady = false;
    lastDataTime = now;
    packetCount++;
    demoActive = false;
    processNewData();
  }

  // --- Source 2: Serial JSON input (Wokwi bridge or direct) ---
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    int start = line.indexOf('{');
    int end = line.lastIndexOf('}');
    if (start >= 0 && end > start) {
      String jsonSub = line.substring(start, end + 1);
      if (parseJsonTelemetry(jsonSub)) {
        lastDataTime = now;
        packetCount++;
        demoActive = false;
        processNewData();
      }
    }
  }

  // --- Source 3: Demo fallback (no data for 5s after boot) ---
  if (!demoActive && now > 5000 && (now - lastDataTime) > 5000) {
    demoActive = true;
    Serial.println("{\"info\":\"No data source detected - entering demo mode\"}");
  }
  if (demoActive && (now - lastDemoTick) >= 500) {
    lastDemoTick = now;
    generateDemoData();
    processNewData();
  }

  // --- Auto-cycle LCD pages every 3s (unless DANGER) ---
  if (telem.state != 3 && (now - lastPageSwitch) >= 3000) {
    lcdPage = (lcdPage + 1) % NUM_PAGES;
    lastPageSwitch = now;
    updateLCD();
  }

  // --- Heartbeat LED off after 80ms blink ---
  if (digitalRead(LED_YELLOW_PIN) == HIGH && (now - lastHeartbeatOn) > 80) {
    digitalWrite(LED_YELLOW_PIN, LOW);
  }

  // --- Link lost check (no data for 3s, only when NOT in demo) ---
  if (!demoActive && linkActive && (now - lastDataTime) > 3000) {
    linkActive = false;
    showLinkLost();
    digitalWrite(LED_GREEN_PIN, LOW);
    // Blink red LED
    digitalWrite(LED_RED_PIN, (now / 500) % 2 == 0 ? HIGH : LOW);
  }
}
