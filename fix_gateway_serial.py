import re

with open("GATEWAY/src/main.cpp", "r") as f:
    content = f.read()

replacement = """
    if (Serial.available()) {
        String line = Serial.readStringUntil('\\n');
        
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
                if (jsonSub.indexOf("\\"type\\":\\"scan\\"") > 0) {
                     if (mqttClient.connected()) mqttClient.publish(MQTT_TOPIC_SCAN, jsonSub.c_str());
                } else {
                     if (mqttClient.connected()) mqttClient.publish(MQTT_TOPIC_TELEMETRY, jsonSub.c_str());
                }
            }
        }
    }
"""

content = re.sub(r'    if \(Serial\.available\(\)\) \{.*?    \}\n', replacement, content, flags=re.DOTALL)

with open("GATEWAY/src/main.cpp", "w") as f:
    f.write(content)
