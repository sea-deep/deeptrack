import re

with open("GATEWAY/src/main.cpp", "r") as f:
    content = f.read()

# Add the LCD logic to the top of GATEWAY/src/main.cpp
lcd_headers = """#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <LiquidCrystal_I2C.h>

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
"""

content = re.sub(r'#include <Arduino\.h>\n#include <WiFi\.h>\n#include <esp_now\.h>\n\n#define USE_ESP_NOW 0\n\n#include <PubSubClient\.h>\n#include "telemetry_packet\.h"', lcd_headers, content)

# LCD Update function
lcd_func = """
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
        Math.atan2(-latestTel.ax, Math.sqrt(latestTel.ay * latestTel.ay + latestTel.az * latestTel.az)) * 180 / Math.PI,
        Math.atan2(latestTel.ay, latestTel.az) * 180 / Math.PI);
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
"""

content = re.sub(r'void onDataRecv\(.*', lcd_func + '\nvoid onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {', content)


# Setup LCD
lcd_setup = """    // LCD init
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
    
    WiFi.mode(WIFI_STA);"""

content = re.sub(r'    WiFi\.mode\(WIFI_STA\);', lcd_setup, content)

# LCD Loop update
lcd_loop = """
    // --- Auto-cycle LCD pages every 3s (unless DANGER) ---
    if (millis() - lastPageChange >= 3000) {
        lastPageChange = millis();
        if (latestTel.dangerState != 1) {
            lcdPage = (lcdPage + 1) % NUM_PAGES;
            updateLCD();
        }
    }
"""

content = re.sub(r'    // Check serial for commands \(direct typing\) or telemetry', lcd_loop + '\n    // Check serial for commands (direct typing) or telemetry', content)

with open("GATEWAY/src/main.cpp", "w") as f:
    f.write(content)
