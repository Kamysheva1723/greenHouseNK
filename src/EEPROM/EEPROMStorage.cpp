#include "EEPROMStorage.h"
#include <hardware/i2c.h>
#include <hardware/gpio.h>
#include <pico/time.h>
#include <cstdio>

/*
   EEPROMStorage Module

   This module provides an abstraction layer to interface with an external I²C EEPROM.
   It handles non-volatile storage operations such as reading and writing bytes, and also
   provides convenience functions for storing and retrieving the CO₂ setpoint which ensures
   that critical parameters persist across power cycles. This is part of the system’s dynamic
   resource allocation and persistence mechanism as described in the project documentation.
*/

EEPROMStorage::EEPROMStorage(i2c_inst_t* i2c_instance, unsigned int sda_pin, unsigned int scl_pin, uint8_t device_address)
        : i2c_instance(i2c_instance), sda_pin(sda_pin), scl_pin(scl_pin), device_address(device_address)
{
    // Configure the SDA and SCL GPIO pins to function as I2C.
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);

    // Enable internal pull-up resistors on SDA and SCL lines as required by I2C standards.
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    // Call the init method to perform any additional initialization.
    init();
}

void EEPROMStorage::init() {
    // Additional initialization code for the EEPROM can be added here if required.
    // Currently, this method is a placeholder for any EEPROM initialization protocols.
}

bool EEPROMStorage::readByte(uint16_t memAddr, uint8_t &data) {
    // Split the 16-bit memory address into two bytes in big-endian format.
    uint8_t addr[2] = { static_cast<uint8_t>(memAddr >> 8),
                        static_cast<uint8_t>(memAddr & 0xFF) };

    // Write the memory address to the EEPROM to set the address pointer.
    // The 'true' parameter indicates a repeated start condition without a STOP.
    int ret = i2c_write_blocking(i2c_instance, device_address, addr, 2, true);
    if (ret != 2) {
        printf("EEPROMStorage: Failed to write address (ret=%d)\n", ret);
        return false;
    }

    // Read one byte of data from the EEPROM corresponding to the given memory address.
    // The 'false' parameter sends a STOP condition after the read.
    ret = i2c_read_blocking(i2c_instance, device_address, &data, 1, false);
    if (ret != 1) {
        printf("EEPROMStorage: Failed to read data (ret=%d)\n", ret);
        return false;
    }

    return true; // Successfully read one byte.
}

bool EEPROMStorage::writeByte(uint16_t memAddr, uint8_t data) {
    // Prepare a buffer containing the 16-bit address (big-endian) followed by the data byte.
    uint8_t buffer[3] = { static_cast<uint8_t>(memAddr >> 8),
                          static_cast<uint8_t>(memAddr & 0xFF),
                          data };

    // Write the address and data to the EEPROM in a single transaction.
    int ret = i2c_write_blocking(i2c_instance, device_address, buffer, 3, false);
    if (ret != 3) {
        printf("EEPROMStorage: Failed to write data (ret=%d)\n", ret);
        return false;
    }

    // Wait approximately 5ms to allow the EEPROM to complete its internal write cycle.
    sleep_ms(5);
    return true;
}

/*
   storeCO2Setpoint():
   This function persists the provided CO₂ setpoint value in the EEPROM.
   It writes the 16-bit CO₂ setpoint value to addresses 0x0000 and 0x0001.
*/
bool EEPROMStorage::storeCO2Setpoint(uint16_t co2ppm) {
    uint8_t data[2] = { static_cast<uint8_t>(co2ppm >> 8),    // Most significant byte
                        static_cast<uint8_t>(co2ppm & 0xFF) };  // Least significant byte
    // Write the high byte to EEPROM at address 0x0000.
    bool ok1 = writeByte(0x0000, data[0]);
    // Write the low byte to EEPROM at address 0x0001.
    bool ok2 = writeByte(0x0001, data[1]);
    return ok1 && ok2;
}

/*
   loadCO2Setpoint():
   This function reads the persisted CO₂ setpoint from the EEPROM.
   It reads two consecutive bytes from addresses 0x0000 and 0x0001, combines them,
   and returns the resulting 16-bit value. If reading fails, it returns a fallback value (1000).
*/
uint16_t EEPROMStorage::loadCO2Setpoint() {
    uint8_t data0 = 0, data1 = 0;
    bool ok1 = readByte(0x0000, data0);
    bool ok2 = readByte(0x0001, data1);
    if (!ok1 || !ok2)
        return 1000; // Fallback value if unable to read from EEPROM.
    return (static_cast<uint16_t>(data0) << 8) | data1;
}