#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "../firmware/shared/DeeptrackHardware.h"
#include "../firmware/shared/DeeptrackProtocol.h"

namespace Pin = DeepTrack::Hardware::Gateway;

LiquidCrystal_I2C lcd(Pin::LCD_ADDRESS, 16, 2);

void allLEDsOff() {
  digitalWrite(Pin::LED_RED, LOW);
  digitalWrite(Pin::LED_GREEN, LOW);
  digitalWrite(Pin::LED_YELLOW, LOW);
}

void testLED(const char* name, uint8_t pin) {
  allLEDsOff();
  digitalWrite(pin, HIGH);

  Serial.print("Testing: ");
  Serial.println(name);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gateway Test");

  lcd.setCursor(0, 1);
  lcd.print(name);

  delay(200);  // was 1000

  digitalWrite(pin, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(Pin::LED_RED, OUTPUT);
  pinMode(Pin::LED_GREEN, OUTPUT);
  pinMode(Pin::LED_YELLOW, OUTPUT);

  allLEDsOff();

  Wire.begin(Pin::SDA, Pin::SCL);

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gateway Boot");
  lcd.setCursor(0, 1);
  lcd.print("Testing...");

  Serial.println("Gateway hardware test");
  Serial.printf("LCD: SDA=%u SCL=%u address=0x%02X\n",
                Pin::SDA, Pin::SCL, Pin::LCD_ADDRESS);

  delay(1500);
}

void loop() {
  testLED("RED - DANGER", Pin::LED_RED);
  testLED("YELLOW - HB", Pin::LED_YELLOW);
  testLED("GREEN - OK", Pin::LED_GREEN);

  allLEDsOff();

  // Normal-state indication
  digitalWrite(Pin::LED_GREEN, HIGH);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gateway Ready");
  lcd.setCursor(0, 1);
  lcd.print("Hardware OK");

  Serial.println("All hardware tested.");
  Serial.printf("RED GPIO%u    : Danger\n", Pin::LED_RED);
  Serial.printf("YELLOW GPIO%u : Heartbeat\n", Pin::LED_YELLOW);
  Serial.printf("GREEN GPIO%u  : Normal\n", Pin::LED_GREEN);

  delay(500); // was 3000

  digitalWrite(Pin::LED_GREEN, LOW);
}
