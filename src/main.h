//
// Created by natal on 23.2.2025.
//

#ifndef MAIN_H
#define MAIN_H

#define ROT_A_PIN   10
#define ROT_B_PIN   11
#define ROT_SW_PIN  12
// Define hardware pins and addresses:
#define EEPROM_DEVICE_ADDRESS 0x50  // 7-bit EEPROM address

// I2C0 for EEPROM
#define EEPROM_SDA_PIN 16
#define EEPROM_SCL_PIN 17

// I2C1 for Display and Pressure sensor
#define I2C1_SDA_PIN 14
#define I2C1_SCL_PIN 15

// Wireless network
#ifdef WIFI_SSID
  #undef WIFI_SSID
#endif
#define WIFI_SSID "SmartIotMQTT"

#ifdef WIFI_PASSWORD
  #undef WIFI_PASSWORD
#endif
#define WIFI_PASSWORD "SmartIot"

#endif
