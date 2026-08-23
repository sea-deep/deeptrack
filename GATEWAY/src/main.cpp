#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>
#include "telemetry_packet.h"

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASS ""
#define MQTT_SERVER "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_TOPIC_TELEMETRY "rover/telemetry"
#define MQTT_TOPIC_SCAN "rover/scan"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

uint8_t roverAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(TelemetryPacket)) {
        TelemetryPacket *pkt = (TelemetryPacket*)incomingData;
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

void reconnectMqtt() {
    if (!mqttClient.connected()) {
        Serial.println("Connecting to MQTT...");
        String clientId = "Gateway-";
        clientId += String(random(0xffff), HEX);
        if (mqttClient.connect(clientId.c_str())) {
            Serial.println("MQTT Connected");
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    // In simulation, wait briefly for WiFi, but don't block ESP-NOW
    for(int i=0; i<10 && WiFi.status() != WL_CONNECTED; i++) {
        delay(500);
    }
    
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    
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
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) reconnectMqtt();
        mqttClient.loop();
    }
    
    // Check serial for commands from the dashboard
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        if (cmd.startsWith("L:")) {
            int spaceIdx = cmd.indexOf(' ');
            if (spaceIdx > -1) {
                ControlPacket pkt;
                pkt.type = 0;
                pkt.motor_l = cmd.substring(2, spaceIdx).toInt();
                pkt.motor_r = cmd.substring(cmd.indexOf("R:") + 2).toInt();
                esp_now_send(roverAddress, (uint8_t*)&pkt, sizeof(ControlPacket));
            }
        }
    }
}
