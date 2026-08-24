#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

#define USE_ESP_NOW 0

#include <PubSubClient.h>
#include "telemetry_packet.h"

// ---------- LCD ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);
uint32_t lastDataTime = 0;
int lcdPage = 0;
#define NUM_PAGES 3
uint32_t lastPageChange = 0;

// Custom Icons
byte thermIcon[8] = { B00100, B01010, B01010, B01110, B01110, B11111, B11111, B01110 };
byte signalIcon[8] = { B00000, B10000, B10100, B10100, B10101, B10101, B10101, B10101 };
byte warnIcon[8] = { B00000, B00100, B01010, B11011, B11011, B11011, B11111, B00000 };

TelemetryPacket latestTel = {0};


#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASS ""
#define MQTT_SERVER "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_TOPIC_TELEMETRY "rover/telemetry"
#define MQTT_TOPIC_SCAN "rover/scan"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

uint8_t roverAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


void updateLCD() {
  lcd.clear();
  
  if (millis() - lastDataTime > 3000) {
      lcd.setCursor(0, 0);
      lcd.write(2); // warning
      lcd.print(" LINK LOST! ");
      lcd.write(2);
      lcd.setCursor(0, 1);
      lcd.print("No data >3s");
      return;
  }
  
  if (latestTel.dangerState == 1) {
      lcd.setCursor(0, 0);
      lcd.write(2); // warning icon
      lcd.print(" DANGER!!  ");
      lcd.write(2);
      lcd.setCursor(0, 1);
      lcd.print("EMERGENCY STOP");
      return;
  }
  
  switch (lcdPage) {
    case 0: { // ENV
      char e1[17], e2[17];
      snprintf(e1, sizeof(e1), " T:%.1fC H:%.1f%%", latestTel.temperature, latestTel.humidity);
      snprintf(e2, sizeof(e2), " Gas:%d W:%d", latestTel.gasRaw, latestTel.waterRaw);
      lcd.setCursor(0, 0); lcd.write(0); lcd.print(e1);
      lcd.setCursor(0, 1); lcd.print(e2);
      break;
    }

    case 1: { // NAV
      char n1[17], n2[17];
      snprintf(n1, sizeof(n1), " P:%.1f R:%.1f", 
        atan2(-latestTel.ax, sqrt(latestTel.ay * latestTel.ay + latestTel.az * latestTel.az)) * 180.0 / 3.14159265,
        atan2(latestTel.ay, latestTel.az) * 180.0 / 3.14159265);
      snprintf(n2, sizeof(n2), " GYR:%d", (int)latestTel.gz);
      lcd.setCursor(0, 0); lcd.print(n1);
      lcd.setCursor(0, 1); lcd.print(n2);
      break;
    }

    case 2: { // SYS
      char s1[17], s2[17];
      snprintf(s1, sizeof(s1), " MQTT:%s", mqttClient.connected() ? "ON" : "OFF");
      snprintf(s2, sizeof(s2), " WiFi:%s", WiFi.status() == WL_CONNECTED ? "ON" : "OFF");
      lcd.setCursor(0, 0); lcd.print(s1);
      lcd.setCursor(0, 1); lcd.write(1); lcd.print(s2);
      break;
    }
  }
}

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(TelemetryPacket)) {
        TelemetryPacket *pkt = (TelemetryPacket*)incomingData;
        memcpy(&latestTel, pkt, sizeof(TelemetryPacket));
        lastDataTime = millis();
        char json[256];
        snprintf(json, sizeof(json),
            "{\"t\":%.1f,\"h\":%.1f,\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f,\"gas\":%d,\"water\":%d,\"danger\":%d}",
            pkt->temperature, pkt->humidity,
            pkt->ax, pkt->ay, pkt->az,
            pkt->gx, pkt->gy, pkt->gz,
            pkt->gasRaw, pkt->waterRaw,
            pkt->dangerState
        );
        Serial.print("TELEMETRY:");
        Serial.println(json);
        if (mqttClient.connected()) {
            mqttClient.publish(MQTT_TOPIC_TELEMETRY, json);
        }
    } else if (len == sizeof(ScanPacket)) {
        ScanPacket *pkt = (ScanPacket*)incomingData;
        lastDataTime = millis();
        char json[128];
        snprintf(json, sizeof(json),
            "{\"type\":\"scan\",\"seq\":%d,\"angle_deg\":%d,\"distance_mm\":%d,\"valid\":%s,\"timestamp_ms\":%d}",
            pkt->seq, pkt->angle_deg, pkt->distance_mm, pkt->valid ? "true" : "false", pkt->timestamp_ms
        );
        Serial.print("SCAN:");
        Serial.println(json);
        if (mqttClient.connected()) {
            mqttClient.publish(MQTT_TOPIC_SCAN, json);
        }
    }
}


void onMqttMessage(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    if (String(topic) == "rover/control") {
        if (msg.startsWith("L:")) {
            int spaceIdx = msg.indexOf(' ');
            if (spaceIdx > -1) {
                ControlPacket pkt;
                pkt.type = 0;
                pkt.motor_l = msg.substring(2, spaceIdx).toInt();
                pkt.motor_r = msg.substring(msg.indexOf("R:") + 2).toInt();
#if USE_ESP_NOW
                esp_now_send(roverAddress, (uint8_t*)&pkt, sizeof(ControlPacket));
#endif
                Serial.println(msg); // Bridge to Wokwi Simulator
            }
        }
    }
}

void reconnectMqtt() {
    if (!mqttClient.connected()) {
        Serial.println("Connecting to MQTT...");
        String clientId = "Gateway-";
        clientId += String(random(0xffff), HEX);
        if (mqttClient.connect(clientId.c_str())) {
            Serial.println("MQTT Connected");
            mqttClient.subscribe("rover/control");
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    // LCD init
    lcd.init();
    lcd.backlight();
    lcd.createChar(0, thermIcon);
    lcd.createChar(1, signalIcon);
    lcd.createChar(2, warnIcon);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DEEPTRACK GTWRY");
    lcd.setCursor(0, 1);
    lcd.print("Waiting link...");
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    // In simulation, wait briefly for WiFi, but don't block ESP-NOW
    for(int i=0; i<10 && WiFi.status() != WL_CONNECTED; i++) {
        delay(500);
    }
    
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(onMqttMessage);
#if USE_ESP_NOW
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Init Failed");
        return;
    }
    esp_now_register_recv_cb(onDataRecv);
    
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, roverAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
#endif
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) reconnectMqtt();
        mqttClient.loop();
    }
    


    // --- Auto-cycle LCD pages every 3s (unless DANGER) ---
    if (millis() - lastPageChange >= 3000) {
        lastPageChange = millis();
        if (latestTel.dangerState != 1) {
            lcdPage = (lcdPage + 1) % NUM_PAGES;
            updateLCD();
        }
    }

    // Check serial for commands (direct typing) or telemetry (from Wokwi Bridge)

    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        
        // Check for commands (direct typing)
        if (line.startsWith("L:")) {
            int spaceIdx = line.indexOf(' ');
            if (spaceIdx > -1) {
                ControlPacket pkt;
                pkt.type = 0;
                pkt.motor_l = line.substring(2, spaceIdx).toInt();
                pkt.motor_r = line.substring(line.indexOf("R:") + 2).toInt();
#if USE_ESP_NOW
                esp_now_send(roverAddress, (uint8_t*)&pkt, sizeof(ControlPacket));
#endif
            }
        } else {

            // Extract JSON telemetry from Wokwi Serial Bridge
            int start = line.indexOf('{');
            int end = line.lastIndexOf('}');
            if (start >= 0 && end > start) {
                String jsonSub = line.substring(start, end + 1);
                
                // Keep the LCD alive
                lastDataTime = millis();
                
                if (jsonSub.indexOf("\"type\":\"scan\"") > 0) {
                     if (mqttClient.connected()) mqttClient.publish(MQTT_TOPIC_SCAN, jsonSub.c_str());
                } else {
                     if (mqttClient.connected()) mqttClient.publish(MQTT_TOPIC_TELEMETRY, jsonSub.c_str());
                     
                     // Parse to update LCD
                     JsonDocument doc;
                     DeserializationError error = deserializeJson(doc, jsonSub);
                     if (!error) {
                         latestTel.temperature = doc["t"] | 0.0f;
                         latestTel.humidity = doc["h"] | 0.0f;
                         latestTel.ax = doc["ax"] | 0.0f;
                         latestTel.ay = doc["ay"] | 0.0f;
                         latestTel.az = doc["az"] | 1.0f;
                         latestTel.gx = doc["gx"] | 0.0f;
                         latestTel.gy = doc["gy"] | 0.0f;
                         latestTel.gz = doc["gz"] | 0.0f;
                         latestTel.gasRaw = doc["gas"] | 0;
                         latestTel.waterRaw = doc["water"] | 0;
                         latestTel.dangerState = doc["danger"] | 0;
                     }
                }
            }
        }
    }

}
