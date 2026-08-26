#include <Arduino.h>

#include "../../firmware/shared/DeeptrackHardware.h"
#include "../../firmware/shared/DeeptrackProtocol.h"

namespace Pin = DeepTrack::Hardware::Rover;

constexpr unsigned long MOTOR_TEST_DURATION_MS = 7000;

unsigned long motorStopAt = 0;
bool motorsRunning = false;

void stopMotors() {
  digitalWrite(Pin::LEFT_PWM, LOW);
  digitalWrite(Pin::RIGHT_PWM, LOW);

  digitalWrite(Pin::LEFT_IN1, LOW);
  digitalWrite(Pin::LEFT_IN2, LOW);

  digitalWrite(Pin::RIGHT_IN1, LOW);
  digitalWrite(Pin::RIGHT_IN2, LOW);

  motorStopAt = 0;
  motorsRunning = false;
}

void setMotorSide(
  uint8_t pwmPin,
  uint8_t in1Pin,
  uint8_t in2Pin,
  int direction
) {
  digitalWrite(pwmPin, LOW);

  if (direction > 0) {
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, LOW);
    digitalWrite(pwmPin, HIGH);
    return;
  }

  if (direction < 0) {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
    digitalWrite(pwmPin, HIGH);
    return;
  }

  digitalWrite(in1Pin, LOW);
  digitalWrite(in2Pin, LOW);
}

void printPins() {
  Serial.println();
  Serial.println("LEFT DRIVER SIGNALS:");

  Serial.printf(
    "GPIO25 / D25 / PWM:  %d\n",
    digitalRead(Pin::LEFT_PWM)
  );

  Serial.printf(
    "GPIO14 / D14 / IN1:  %d\n",
    digitalRead(Pin::LEFT_IN1)
  );

  Serial.printf(
    "GPIO16 / RX2 / IN2:  %d\n",
    digitalRead(Pin::LEFT_IN2)
  );

  Serial.println("RIGHT DRIVER SIGNALS:");

  Serial.printf(
    "GPIO17 / TX2 / PWM:  %d\n",
    digitalRead(Pin::RIGHT_PWM)
  );

  Serial.printf(
    "GPIO33 / D33 / IN1:  %d\n",
    digitalRead(Pin::RIGHT_IN1)
  );

  Serial.printf(
    "GPIO32 / D32 / IN2:  %d\n",
    digitalRead(Pin::RIGHT_IN2)
  );

  Serial.println();
}

void runMotors(
  int leftDirection,
  int rightDirection,
  const char *description
) {
  stopMotors();

  setMotorSide(
    Pin::LEFT_PWM,
    Pin::LEFT_IN1,
    Pin::LEFT_IN2,
    leftDirection
  );

  setMotorSide(
    Pin::RIGHT_PWM,
    Pin::RIGHT_IN1,
    Pin::RIGHT_IN2,
    rightDirection
  );

  motorsRunning = true;
  motorStopAt = millis() + MOTOR_TEST_DURATION_MS;

  Serial.printf(
    "\nRUNNING: %s for %lu seconds\n",
    description,
    MOTOR_TEST_DURATION_MS / 1000
  );

  printPins();
}

void printHelp() {
  Serial.println();
  Serial.println("=== TB6612 MOTOR DRIVER TEST ===");
  Serial.println("Commands:");
  Serial.println("  left      - left wheels forward");
  Serial.println("  leftrev   - left wheels backward");
  Serial.println("  right     - right wheels forward");
  Serial.println("  rightrev  - right wheels backward");
  Serial.println("  forward   - both sides forward");
  Serial.println("  back      - both sides backward");
  Serial.println("  pins      - show current GPIO states");
  Serial.println("  stop      - stop immediately");
  Serial.println("  help      - show commands");
  Serial.println();
  Serial.println("Each motor command runs for 7 seconds.");
  Serial.println("Keep the wheels lifted during testing.");
}

void setup() {
  Serial.begin(115200);

  pinMode(Pin::LEFT_PWM, OUTPUT);
  pinMode(Pin::LEFT_IN1, OUTPUT);
  pinMode(Pin::LEFT_IN2, OUTPUT);

  pinMode(Pin::RIGHT_PWM, OUTPUT);
  pinMode(Pin::RIGHT_IN1, OUTPUT);
  pinMode(Pin::RIGHT_IN2, OUTPUT);

  stopMotors();

  delay(500);

  printHelp();
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');

    command.trim();
    command.toLowerCase();

    if (command == "left") {
      runMotors(1, 0, "LEFT FORWARD");
    }

    else if (command == "leftrev") {
      runMotors(-1, 0, "LEFT REVERSE");
    }

    else if (command == "right") {
      runMotors(0, 1, "RIGHT FORWARD");
    }

    else if (command == "rightrev") {
      runMotors(0, -1, "RIGHT REVERSE");
    }

    else if (command == "forward") {
      runMotors(1, 1, "BOTH FORWARD");
    }

    else if (command == "back") {
      runMotors(-1, -1, "BOTH REVERSE");
    }

    else if (command == "stop") {
      stopMotors();
      Serial.println("STOPPED");
    }

    else if (command == "pins") {
      printPins();
    }

    else if (command == "help") {
      printHelp();
    }

    else if (command.length() > 0) {
      Serial.printf(
        "Unknown command: %s\n",
        command.c_str()
      );
    }
  }

  if (
    motorsRunning &&
    (long)(millis() - motorStopAt) >= 0
  ) {
    stopMotors();
    Serial.println("7 seconds completed. Motors stopped.");
  }

  delay(5);
}
