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

  // Range Millimeter High and Low bytes in result buffer
  chip->registers[0x1E] = (uint8_t)((distance_mm >> 8) & 0xFF);
  chip->registers[0x1F] = (uint8_t)(distance_mm & 0xFF);

  // Status: Sample Ready (bit 2 or bit 0 set, 0x04 / 0x07)
  chip->registers[REG_RESULT_INTERRUPT_STATUS] = 0x07;
  chip->registers[REG_RESULT_RANGE_STATUS] = 0x00; // Device ready / Valid measurement
}

static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
  chip_state_t *chip = (chip_state_t *)user_data;
  
  if (read) {
    // When master starts a read, update sensor register data
    update_measurement_registers(chip);
  } else {
    // Starting a write sequence: the first byte will be the register pointer
    chip->is_first_write_byte = true;
  }
  
  return true; // ACK address
}

static uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  
  // Return current register byte and auto-increment pointer
  uint8_t value = chip->registers[chip->reg_ptr];
  chip->reg_ptr++;
  return value;
}

static bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = (chip_state_t *)user_data;

  if (chip->is_first_write_byte) {
    // First byte after connect is register address index
    chip->reg_ptr = data;
    chip->is_first_write_byte = false;
  } else {
    // Subsequent bytes are data written to the register
    chip->registers[chip->reg_ptr] = data;
    
    // Handle triggers (e.g. Sysrange Start)
    if (chip->reg_ptr == REG_SYSRANGE_START) {
      update_measurement_registers(chip);
    }
    
    chip->reg_ptr++;
  }
  
  return true; // ACK byte
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
  chip->registers[0x51] = 0x00; // Expected Model ID check 0x0099
  chip->registers[0x52] = 0x99;
  chip->registers[0x61] = 0x00;
  chip->registers[0x62] = 0x00;
  chip->registers[0x88] = 0x00;
  chip->registers[0x89] = 0x00;
  chip->registers[REG_RESULT_INTERRUPT_STATUS] = 0x07;
  
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
