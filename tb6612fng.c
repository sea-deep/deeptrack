#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
  // Input logic & control pins
  pin_t pin_ain1;
  pin_t pin_ain2;
  pin_t pin_pwma;
  pin_t pin_bin1;
  pin_t pin_bin2;
  pin_t pin_pwmb;
  pin_t pin_stby;
  
  // Power pins (optional monitoring)
  pin_t pin_vm;
  pin_t pin_vcc;

  // Output drive pins
  pin_t pin_ao1;
  pin_t pin_ao2;
  pin_t pin_bo1;
  pin_t pin_bo2;
} chip_state_t;

// Update H-bridge outputs based on inputs according to TB6612FNG truth table
static void update_motor_channel(uint32_t in1, uint32_t in2, uint32_t pwm, pin_t out1, pin_t out2) {
  if (pwm == LOW) {
    // PWM LOW -> Short brake / stop
    pin_mode(out1, OUTPUT);
    pin_mode(out2, OUTPUT);
    pin_write(out1, LOW);
    pin_write(out2, LOW);
  } else if (in1 == HIGH && in2 == LOW) {
    // CW / Forward
    pin_mode(out1, OUTPUT);
    pin_mode(out2, OUTPUT);
    pin_write(out1, HIGH);
    pin_write(out2, LOW);
  } else if (in1 == LOW && in2 == HIGH) {
    // CCW / Reverse
    pin_mode(out1, OUTPUT);
    pin_mode(out2, OUTPUT);
    pin_write(out1, LOW);
    pin_write(out2, HIGH);
  } else if (in1 == HIGH && in2 == HIGH) {
    // Short Brake
    pin_mode(out1, OUTPUT);
    pin_mode(out2, OUTPUT);
    pin_write(out1, LOW);
    pin_write(out2, LOW);
  } else {
    // in1 == LOW && in2 == LOW -> Coast / High impedance
    pin_mode(out1, INPUT);
    pin_mode(out2, INPUT);
  }
}

static void update_all_outputs(chip_state_t *chip) {
  uint32_t stby = pin_read(chip->pin_stby);

  if (stby == LOW) {
    // Standby mode -> High impedance (Hi-Z) on all motor outputs
    pin_mode(chip->pin_ao1, INPUT);
    pin_mode(chip->pin_ao2, INPUT);
    pin_mode(chip->pin_bo1, INPUT);
    pin_mode(chip->pin_bo2, INPUT);
    return;
  }

  // Read Channel A inputs
  uint32_t ain1 = pin_read(chip->pin_ain1);
  uint32_t ain2 = pin_read(chip->pin_ain2);
  uint32_t pwma = pin_read(chip->pin_pwma);
  update_motor_channel(ain1, ain2, pwma, chip->pin_ao1, chip->pin_ao2);

  // Read Channel B inputs
  uint32_t bin1 = pin_read(chip->pin_bin1);
  uint32_t bin2 = pin_read(chip->pin_bin2);
  uint32_t pwmb = pin_read(chip->pin_pwmb);
  update_motor_channel(bin1, bin2, pwmb, chip->pin_bo1, chip->pin_bo2);
}

// Callback invoked on any input pin state transition
static void on_pin_change(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *chip = (chip_state_t *)user_data;
  update_all_outputs(chip);
}

void chip_init(void) {
  chip_state_t *chip = (chip_state_t *)malloc(sizeof(chip_state_t));

  // Initialize input logic pins with pull-down resistors (TB6612 default behavior)
  chip->pin_ain1 = pin_init("AIN1", INPUT_PULLDOWN);
  chip->pin_ain2 = pin_init("AIN2", INPUT_PULLDOWN);
  chip->pin_pwma = pin_init("PWMA", INPUT_PULLDOWN);
  chip->pin_bin1 = pin_init("BIN1", INPUT_PULLDOWN);
  chip->pin_bin2 = pin_init("BIN2", INPUT_PULLDOWN);
  chip->pin_pwmb = pin_init("PWMB", INPUT_PULLDOWN);
  chip->pin_stby = pin_init("STBY", INPUT_PULLDOWN);

  // Initialize power pins
  chip->pin_vm  = pin_init("VM", INPUT);
  chip->pin_vcc = pin_init("VCC", INPUT);

  // Initialize output pins (initially in high-Z/off state)
  chip->pin_ao1 = pin_init("AO1", INPUT);
  chip->pin_ao2 = pin_init("AO2", INPUT);
  chip->pin_bo1 = pin_init("BO1", INPUT);
  chip->pin_bo2 = pin_init("BO2", INPUT);

  // Configure pin watch for state change interrupts on all control pins
  const pin_watch_config_t watch_config = {
    .edge = BOTH,
    .pin_change = on_pin_change,
    .user_data = chip,
  };

  pin_watch(chip->pin_ain1, &watch_config);
  pin_watch(chip->pin_ain2, &watch_config);
  pin_watch(chip->pin_pwma, &watch_config);
  pin_watch(chip->pin_bin1, &watch_config);
  pin_watch(chip->pin_bin2, &watch_config);
  pin_watch(chip->pin_pwmb, &watch_config);
  pin_watch(chip->pin_stby, &watch_config);

  // Initial evaluation
  update_all_outputs(chip);
}
