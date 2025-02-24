#ifndef EEPROM_STORAGE_H
#define EEPROM_STORAGE_H

#include <cstdint>
#include "hardware/i2c.h"  // For i2c_inst_t

/**
 * @brief EEPROMStorage provides a simple interface to read and write one byte at a time
 *        from an I²C EEPROM using the Pico SDK's hardware I²C API.
 *
 * It uses a 16-bit memory address (sent in big-endian order) and a 7-bit I²C device address.
 * In addition to readByte() and writeByte(), it provides convenience functions to store
 * and load a CO₂ setpoint.
 */
class EEPROMStorage {
public:
    /**
     * @brief Constructor.
     * @param i2c_instance Pointer to the I2C hardware instance (e.g. i2c0 or i2c1).
     * @param sda_pin The GPIO pin used for I²C SDA.
     * @param scl_pin The GPIO pin used for I²C SCL.
     * @param device_address The 7-bit I²C device address of the EEPROM.
     */
    EEPROMStorage(i2c_inst_t* i2c_instance, unsigned int sda_pin, unsigned int scl_pin, uint8_t device_address);

    bool readByte(uint16_t memAddr, uint8_t &data);
    bool writeByte(uint16_t memAddr, uint8_t data);
    bool storeCO2Setpoint(uint16_t co2ppm);
    uint16_t loadCO2Setpoint();

private:
    i2c_inst_t* i2c_instance;
    unsigned int sda_pin;
    unsigned int scl_pin;
    uint8_t device_address;

    void init();
};

#endif // EEPROM_STORAGE_H

