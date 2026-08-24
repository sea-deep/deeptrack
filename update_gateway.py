import re

with open("GATEWAY/src/main.cpp", "r") as f:
    content = f.read()

# Add MQTT callback and subscription
mqtt_additions = """
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
                esp_now_send(roverAddress, (uint8_t*)&pkt, sizeof(ControlPacket));
                Serial.println(msg); // Bridge to Wokwi Simulator
            }
        }
    }
}
"""

content = re.sub(r'void reconnectMqtt\(\) \{', mqtt_additions + r'\nvoid reconnectMqtt() {', content)
content = re.sub(r'mqttClient\.setServer\(MQTT_SERVER, MQTT_PORT\);', r'mqttClient.setServer(MQTT_SERVER, MQTT_PORT);\n    mqttClient.setCallback(onMqttMessage);', content)
content = re.sub(r'Serial\.println\("MQTT Connected"\);', r'Serial.println("MQTT Connected");\n            mqttClient.subscribe("rover/control");', content)

# Modify Serial reading to handle incoming JSON from Rover
serial_read_replacement = """
    // Check serial for commands (direct typing) or telemetry (from Wokwi Bridge)
    if (Serial.available()) {
        String line = Serial.readStringUntil('\\n');
        if (line.startsWith("L:")) {
            int spaceIdx = line.indexOf(' ');
            if (spaceIdx > -1) {
                ControlPacket pkt;
                pkt.type = 0;
                pkt.motor_l = line.substring(2, spaceIdx).toInt();
                pkt.motor_r = line.substring(line.indexOf("R:") + 2).toInt();
                esp_now_send(roverAddress, (uint8_t*)&pkt, sizeof(ControlPacket));
            }
        } else if (line.startsWith("{")) {
            // It's telemetry from Wokwi Serial Bridge!
            if (line.indexOf("\\"type\\":\\"scan\\"") > 0) {
                 if (mqttClient.connected()) mqttClient.publish(MQTT_TOPIC_SCAN, line.c_str());
            } else {
                 if (mqttClient.connected()) mqttClient.publish(MQTT_TOPIC_TELEMETRY, line.c_str());
            }
        }
    }
"""

# Replace the existing serial reading block in loop()
content = re.sub(r'    // Check serial for commands from the dashboard.*?\}\n    \}', serial_read_replacement, content, flags=re.DOTALL)

with open("GATEWAY/src/main.cpp", "w") as f:
    f.write(content)
