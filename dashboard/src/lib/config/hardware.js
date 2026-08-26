// UI projection of docs/WIRING_GUIDE.md and firmware/shared/DeeptrackHardware.h.
// "Reference" means the connection is declared by the locked wiring plan; it
// does not claim that a component is powered, detected, or healthy at runtime.
export const wiringAuthority = Object.freeze({
  document: 'docs/WIRING_GUIDE.md',
  firmwareHeader: 'firmware/shared/DeeptrackHardware.h',
  statusMeaning: 'Reference only — runtime health is not yet connected'
});

export const hardwareInventory = Object.freeze([
  { component: 'ESP32 Gateway', type: 'MCU', interface: 'USB serial / ESP-NOW', pin: 'USB + radio', status: 'Reference' },
  { component: 'ESP32 Rover', type: 'MCU', interface: 'ESP-NOW', pin: 'ADC1 sensors', status: 'Reference' },
  { component: '2× TB6612FNG', type: 'Motor', interface: 'GPIO', pin: 'STBY: 3V3 hardwired', status: 'Reference' },
  { component: 'Left motor bank', type: 'Actuator', interface: 'PWM', pin: 'PWM:25 IN1:14 IN2:16', status: 'Reference' },
  { component: 'Right motor bank', type: 'Actuator', interface: 'PWM', pin: 'PWM:17 IN1:33 IN2:32', status: 'Reference' },
  { component: 'LM393 encoder L', type: 'Sensor', interface: 'Digital input', pin: 'GPIO34 + 10k pull-up', status: 'Reference' },
  { component: 'LM393 encoder R', type: 'Sensor', interface: 'Digital input', pin: 'GPIO35 + 10k pull-up', status: 'Reference' },
  { component: 'MPU6050', type: 'Sensor', interface: 'I2C 0x68', pin: 'SDA:21 SCL:22', status: 'Reference' },
  { component: 'VL53L0X ToF', type: 'Sensor', interface: 'I2C 0x29', pin: 'SDA:21 SCL:22', status: 'Reference' },
  { component: 'HC-SR04', type: 'Sensor', interface: 'GPIO', pin: 'TRIG:19 ECHO:18', status: 'Reference' },
  { component: 'MQ-4 module', type: 'Sensor', interface: 'ADC1', pin: 'GPIO36 via 10k/15k', status: 'Reference' },
  { component: 'DHT22', type: 'Sensor', interface: 'Digital', pin: 'GPIO23', status: 'Reference' },
  { component: 'Water contact probe', type: 'Sensor', interface: 'ADC1', pin: 'GPIO39', status: 'Reference' },
  { component: 'SG90 scanner servo', type: 'Actuator', interface: 'PWM', pin: 'GPIO13', status: 'Reference' },
  { component: 'Rover buzzer', type: 'Actuator', interface: 'GPIO', pin: 'GPIO4', status: 'Reference' },
  { component: 'Rover status LEDs', type: 'Indicator', interface: 'GPIO', pin: 'R:26 G:27', status: 'Reference' },
  { component: 'Gateway LCD', type: 'Indicator', interface: 'I2C 0x27', pin: 'SDA:21 SCL:22', status: 'Reference' },
  { component: 'Gateway status LEDs', type: 'Indicator', interface: 'GPIO', pin: 'R:25 Y:26 G:27', status: 'Reference' }
]);
