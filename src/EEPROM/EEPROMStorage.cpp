#include "EEPROMStorage.h"
#include <hardware/i2c.h>
#include <hardware/gpio.h>
#include <pico/time.h>
#include <cstdio>

EEPROMStorage::EEPROMStorage(i2c_inst_t* i2c_instance, unsigned int sda_pin, unsigned int scl_pin, uint8_t device_address)
        : i2c_instance(i2c_instance), sda_pin(sda_pin), scl_pin(scl_pin), device_address(device_address)
{
    // Configure the GPIO pins for I2C
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);

    // Enable internal pull-ups on SDA and SCL.
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    init();
}

void EEPROMStorage::init() {
    // Optionally, add any additional initialization code here.
}

bool EEPROMStorage::readByte(uint16_t memAddr, uint8_t &data) {
    // Split the 16-bit memory address into two bytes (big-endian order).
    uint8_t addr[2] = { static_cast<uint8_t>(memAddr >> 8),
                        static_cast<uint8_t>(memAddr & 0xFF) };

    // Write the address pointer to the EEPROM.
    // The 'true' parameter requests a repeated start (i.e., no STOP condition is sent).
    int ret = i2c_write_blocking(i2c_instance, device_address, addr, 2, true);
    if (ret != 2) {
        printf("EEPROMStorage: Failed to write address (ret=%d)\n", ret);
        return false;
    }

    // Read one byte from the EEPROM.
    // The 'false' parameter ensures a STOP condition is sent after reading.
    ret = i2c_read_blocking(i2c_instance, device_address, &data, 1, false);
    if (ret != 1) {
        printf("EEPROMStorage: Failed to read data (ret=%d)\n", ret);
        return false;
    }

    return true;
}

bool EEPROMStorage::writeByte(uint16_t memAddr, uint8_t data) {
    // Prepare a 3-byte buffer: two bytes for the memory address and one for the data.
    uint8_t buffer[3] = { static_cast<uint8_t>(memAddr >> 8),
                          static_cast<uint8_t>(memAddr & 0xFF),
                          data };

    // Write the buffer to the EEPROM.
    int ret = i2c_write_blocking(i2c_instance, device_address, buffer, 3, false);
    if (ret != 3) {
        printf("EEPROMStorage: Failed to write data (ret=%d)\n", ret);
        return false;
    }

    // Wait for the EEPROM to complete its internal write cycle (typically ~5ms).
    sleep_ms(5);
    return true;
}

bool EEPROMStorage::storeCO2Setpoint(uint16_t co2ppm) {
    uint8_t data[2] = { static_cast<uint8_t>(co2ppm >> 8),
                        static_cast<uint8_t>(co2ppm & 0xFF) };
    // Write the two bytes at consecutive addresses: 0x0000 and 0x0001.
    bool ok1 = writeByte(0x0000, data[0]);
    bool ok2 = writeByte(0x0001, data[1]);
    return ok1 && ok2;
}

uint16_t EEPROMStorage::loadCO2Setpoint() {
    uint8_t data0 = 0, data1 = 0;
    bool ok1 = readByte(0x0000, data0);
    bool ok2 = readByte(0x0001, data1);
    if (!ok1 || !ok2)
        return 1000; // fallback value
    return (static_cast<uint16_t>(data0) << 8) | data1;
}
