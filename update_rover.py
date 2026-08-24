import re

with open("src/main.cpp", "r") as f:
    content = f.read()

# Add Serial reading to loop()
loop_addition = """
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
"""
content = re.sub(r'void loop\(\) \{(\s+)uint32_t now = millis\(\);', r'void loop() {\1uint32_t now = millis();\n' + loop_addition, content)

# Add JSON print to scan
scan_json = """
        esp_now_send(gatewayAddress, (uint8_t *) &spkt, sizeof(ScanPacket));
        
        char json[128];
        snprintf(json, sizeof(json),
            "{\\"type\\":\\"scan\\",\\"seq\\":%d,\\"angle_deg\\":%d,\\"distance_mm\\":%d,\\"valid\\":%s,\\"timestamp_ms\\":%u}",
            spkt.seq, spkt.angle_deg, spkt.distance_mm, spkt.valid ? "true" : "false", spkt.timestamp_ms
        );
        Serial.println(json);
"""
content = re.sub(r'esp_now_send\(gatewayAddress, \(uint8_t \*\) &spkt, sizeof\(ScanPacket\)\);', scan_json, content)

# Add JSON print to telemetry
telemetry_json = """
        esp_now_send(gatewayAddress, (uint8_t *) &currentTelemetry, sizeof(TelemetryPacket));
        
        char json[256];
        snprintf(json, sizeof(json),
            "{\\"t\\":%.1f,\\"h\\":%.1f,\\"ax\\":%.2f,\\"ay\\":%.2f,\\"az\\":%.2f,\\"gx\\":%.2f,\\"gy\\":%.2f,\\"gz\\":%.2f,\\"gas\\":%d,\\"water\\":%d,\\"danger\\":%d}",
            currentTelemetry.temperature, currentTelemetry.humidity,
            currentTelemetry.ax, currentTelemetry.ay, currentTelemetry.az,
            currentTelemetry.gx, currentTelemetry.gy, currentTelemetry.gz,
            currentTelemetry.gasRaw, currentTelemetry.waterRaw,
            currentTelemetry.dangerState
        );
        Serial.println(json);
"""
content = re.sub(r'esp_now_send\(gatewayAddress, \(uint8_t \*\) &currentTelemetry, sizeof\(TelemetryPacket\)\);', telemetry_json, content)

with open("src/main.cpp", "w") as f:
    f.write(content)
