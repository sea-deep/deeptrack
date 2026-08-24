import re
with open("GATEWAY/src/main.cpp", "r") as f:
    c = f.read()

# Replace the math block entirely because ESP32 Arduino framework uses standard C <math.h> functions
replacement = """
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
"""

c = re.sub(r'    case 1: \{.*?    \}', replacement, c, flags=re.DOTALL)
with open("GATEWAY/src/main.cpp", "w") as f:
    f.write(c)
