//
// Created by natal on 23.2.2025.
//
/*
   This file defines the hardware pin assignments and I2C address configurations
   for the Greenhouse Fertilization System. The definitions provided below are used
   by various modules in the system (e.g., rotary encoder, EEPROM storage, OLED display,
   and pressure sensor) and reflect the wiring diagram and system architecture specified
   in the project documentation.
*/

// Rotary Encoder Pin Definitions:
// These pins are used by the rotary encoder for user input on setting the CO₂ setpoint.
#define ROT_A_PIN   10   // Pin for rotary encoder signal A (primary quadrature signal)
#define ROT_B_PIN   11   // Pin for rotary encoder signal B (used to determine rotation direction)
#define ROT_SW_PIN  12   // Pin for the rotary encoder push-button (with internal pull-up enabled)

// EEPROM Configuration:
// The external EEPROM, which is used to persist critical parameters (e.g., CO₂ setpoint),
// is connected on I2C bus 0.
#define EEPROM_DEVICE_ADDRESS 0x50  // 7-bit I2C address of the EEPROM module

// I2C0 Bus Pins for EEPROM:
// These pins are dedicated to the EEPROM module, ensuring non-volatile storage of settings.
#define EEPROM_SDA_PIN 16  // Pin used for I²C0 data line (SDA) for EEPROM communication
#define EEPROM_SCL_PIN 17  // Pin used for I²C0 clock line (SCL) for EEPROM communication

// I2C1 Bus Pins for Display and Pressure Sensor:
// These pins are allocated for peripherals that share the I2C1 bus, including the SSD1306 OLED
// display for UI feedback and the pressure sensor used for environmental monitoring.
#define I2C1_SDA_PIN 14    // Pin used for I²C1 data line (SDA) for display and pressure sensor communication
#define I2C1_SCL_PIN 15    // Pin used for I²C1 clock line (SCL) for display and pressure sensor communication