#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SDA_PIN 21
#define SCL_PIN 22

#define LED_RED     26   // Danger
#define LED_GREEN   27   // Normal
#define LED_YELLOW  25   // Heartbeat

LiquidCrystal_I2C lcd(0x27, 16, 2);

void allLEDsOff() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
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

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);

  allLEDsOff();

  Wire.begin(SDA_PIN, SCL_PIN);

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gateway Boot");
  lcd.setCursor(0, 1);
  lcd.print("Testing...");

  Serial.println("Gateway hardware test");
  Serial.println("LCD: SDA=21 SCL=22");

  delay(1500);
}

void loop() {
  testLED("RED - DANGER", LED_RED);
  testLED("YELLOW - HB", LED_YELLOW);
  testLED("GREEN - OK", LED_GREEN);

  allLEDsOff();

  // Normal-state indication
  digitalWrite(LED_GREEN, HIGH);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gateway Ready");
  lcd.setCursor(0, 1);
  lcd.print("Hardware OK");

  Serial.println("All hardware tested.");
  Serial.println("RED GPIO26    : Danger");
  Serial.println("GREEN GPIO27  : Normal");
  Serial.println("YELLOW GPIO25 : Heartbeat");

  delay(500); // was 3000

  digitalWrite(LED_GREEN, LOW);
}
