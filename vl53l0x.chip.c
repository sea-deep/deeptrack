#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define VL53L0X_DEFAULT_I2C_ADDR 0x29

// Key VL53L0X Register Addresses
#define REG_IDENTIFICATION_MODEL_ID     0xC0  // Expected default: 0xEE
#define REG_IDENTIFICATION_REVISION_ID  0xC2  // Expected default: 0x10 / 0xAA
#define REG_SYSRANGE_START              0x00
#define REG_RESULT_INTERRUPT_STATUS     0x13
#define REG_RESULT_RANGE_STATUS         0x14  // 0x14 + 10 (0x1E) is Range mm MSB, 0x1F is LSB

typedef struct {
  pin_t scl_pin;
  pin_t sda_pin;
  pin_t xshut_pin;
  pin_t gpio1_pin;

  uint32_t distance_attr;
  
  uint8_t registers[256];
  uint8_t reg_ptr;
  bool is_first_write_byte;
} chip_state_t;

static void update_measurement_registers(chip_state_t *chip) {
  // Read distance from Wokwi UI control slider (in mm)
  uint32_t distance_mm = attr_read(chip->distance_attr);
  if (distance_mm > 8191) distance_mm = 8191; // 13-bit max range

  // Range Millimeter High and Low bytes in result buffer (0x14 + 10 = 0x1E)
  chip->registers[0x1E] = (uint8_t)((distance_mm >> 8) & 0xFF);
  chip->registers[0x1F] = (uint8_t)(distance_mm & 0xFF);

  // Status: Sample Ready (0x04 = NEW_SAMPLE_READY)
  chip->registers[REG_RESULT_INTERRUPT_STATUS] = 0x04;
  chip->registers[REG_RESULT_RANGE_STATUS] = 0x01;
}

static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (!read) {
    chip->is_first_write_byte = true;
  }
  return true;
}

static uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint8_t value = chip->registers[chip->reg_ptr];
  
  if (chip->reg_ptr == REG_SYSRANGE_START) {
    value = 0x00;
  } else if (chip->reg_ptr == 0x83) {
    value = 0x01;
  }

  chip->reg_ptr++;
  return value;
}

static bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = (chip_state_t *)user_data;

  if (chip->is_first_write_byte) {
    chip->reg_ptr = data;
    chip->is_first_write_byte = false;
  } else {
    chip->registers[chip->reg_ptr] = data;
    
    if (chip->reg_ptr == 0x0B) { // SYSTEM_INTERRUPT_CLEAR
      chip->registers[REG_RESULT_INTERRUPT_STATUS] = 0x00;
      chip->registers[REG_RESULT_RANGE_STATUS] = 0x00;
    } else if (chip->reg_ptr == REG_SYSRANGE_START) {
      update_measurement_registers(chip);
      chip->registers[REG_RESULT_INTERRUPT_STATUS] = 0x04;
      chip->registers[REG_RESULT_RANGE_STATUS] = 0x01;
    } else if (chip->reg_ptr == 0x94) { // NVM Read selector
      if (data == 0x6B) {
        chip->registers[0x90] = 0x00;
        chip->registers[0x91] = 0x00;
        chip->registers[0x92] = 0x84; // 4 aperture SPADs
        chip->registers[0x93] = 0x00;
      } else {
        chip->registers[0x90] = 0xFF; // All good SPADs
        chip->registers[0x91] = 0xFF;
        chip->registers[0x92] = 0xFF;
        chip->registers[0x93] = 0xFF;
      }
    }
    
    chip->reg_ptr++;
  }
  
  return true;
}

static void on_i2c_disconnect(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  chip->is_first_write_byte = true;
}

void chip_init(void) {
  chip_state_t *chip = (chip_state_t *)malloc(sizeof(chip_state_t));
  memset(chip, 0, sizeof(chip_state_t));

  // Initialize interactive attribute (default 250 mm)
  chip->distance_attr = attr_init("distance", 250);

  // Initialize pins
  chip->scl_pin = pin_init("SCL", INPUT);
  chip->sda_pin = pin_init("SDA", INPUT);
  chip->xshut_pin = pin_init("XSHUT", INPUT_PULLUP);
  chip->gpio1_pin = pin_init("GPIO1", OUTPUT);
  pin_write(chip->gpio1_pin, LOW);

  // Prepopulate standard VL53L0X default registers
  chip->registers[REG_IDENTIFICATION_MODEL_ID] = 0xEE;
  chip->registers[0xC1] = 0xAA;
  chip->registers[REG_IDENTIFICATION_REVISION_ID] = 0x10;
  chip->registers[0x50] = 0x07; // Pre-range VCSEL period
  chip->registers[0x70] = 0x05; // Final-range VCSEL period
  chip->registers[0x83] = 0x00;
  chip->registers[0x84] = 0x01; // Osc frequency (MSB)
  chip->registers[0x85] = 0x00; // Osc frequency (LSB)
  chip->registers[0x88] = 0x00;
  chip->registers[0x89] = 0x00;
  chip->registers[0x91] = 0x3C; // Stop variable
  chip->registers[REG_RESULT_INTERRUPT_STATUS] = 0x04;
  chip->registers[REG_RESULT_RANGE_STATUS] = 0x01;
  
  // Initial measurement values
  update_measurement_registers(chip);

  // Configure and register I2C slave interface
  const i2c_config_t i2c_config = {
    .user_data = chip,
    .address = VL53L0X_DEFAULT_I2C_ADDR,
    .scl = chip->scl_pin,
    .sda = chip->sda_pin,
    .connect = on_i2c_connect,
    .read = on_i2c_read,
    .write = on_i2c_write,
    .disconnect = on_i2c_disconnect
  };

  i2c_init(&i2c_config);
  printf("VL53L0X Custom Chip initialized on I2C address 0x%02X\n", VL53L0X_DEFAULT_I2C_ADDR);
}
