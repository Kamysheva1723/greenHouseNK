#ifndef EEPROM_STORAGE_H
#define EEPROM_STORAGE_H

#include <cstdint>
#include "hardware/i2c.h"  // Provides definition for i2c_inst_t used for I2C communication

/*
   EEPROMStorage Class

   This class encapsulates the low-level operations needed to interface with an external I²C EEPROM.
   It provides methods for reading and writing a single byte at a time using a 16-bit memory address
   (sent in big-endian order). Furthermore, it offers convenience functions for storing and loading the
   CO₂ setpoint, which is critical for maintaining persistent system settings across device resets.
*/
class EEPROMStorage {
public:
    /**
     * @brief Constructor for EEPROMStorage.
     *
     * Initializes the EEPROM interface by setting up the I²C hardware pins (SDA and SCL) to operate
     * in I2C mode with internal pull-ups enabled. It then calls the init() method to perform any additional
     * post-configuration steps.
     *
     * @param i2c_instance Pointer to the I2C hardware instance (e.g., i2c0 or i2c1) used for communication.
     * @param sda_pin GPIO pin number used for the I²C SDA line.
     * @param scl_pin GPIO pin number used for the I²C SCL line.
     * @param device_address The 7-bit I²C address of the EEPROM device.
     */
    EEPROMStorage(i2c_inst_t* i2c_instance, unsigned int sda_pin, unsigned int scl_pin, uint8_t device_address);

    /**
     * @brief Reads a single byte from the EEPROM.
     *
     * Writes the 16-bit memory address to the EEPROM to set the internal address pointer (using a repeated start)
     * and then reads one byte from that location.
     *
     * @param memAddr The 16-bit memory address to read from.
     * @param data Output parameter where the read byte will be stored.
     * @return true on success, false otherwise.
     */
    bool readByte(uint16_t memAddr, uint8_t &data);

    /**
     * @brief Writes a single byte to the EEPROM.
     *
     * Sends a 3-byte buffer containing the 16-bit memory address (in big-endian order) followed by the data byte.
     * Afterwards, it waits a few milliseconds to allow the EEPROM to complete its internal write cycle.
     *
     * @param memAddr The 16-bit memory address to write to.
     * @param data The byte value to be written.
     * @return true on successful write, false otherwise.
     */
    bool writeByte(uint16_t memAddr, uint8_t data);

    /**
     * @brief Persists the CO₂ setpoint by writing its 16-bit value to EEPROM.
     *
     * The CO₂ setpoint is stored at two consecutive memory addresses (0x0000 and 0x0001) in big-endian order.
     *
     * @param co2ppm The CO₂ setpoint value (in ppm) to store.
     * @return true if both bytes are written successfully, false otherwise.
     */
    bool storeCO2Setpoint(uint16_t co2ppm);

    /**
     * @brief Loads the persisted CO₂ setpoint from EEPROM.
     *
     * Reads two consecutive bytes from addresses 0x0000 and 0x0001 and combines them to form the 16-bit setpoint.
     * Returns a fallback value (1000 ppm) if reading fails.
     *
     * @return The 16-bit CO₂ setpoint value.
     */
    uint16_t loadCO2Setpoint();

private:
    i2c_inst_t* i2c_instance;  // The I2C instance used for EEPROM communication.
    unsigned int sda_pin;      // GPIO pin for the I²C data line.
    unsigned int scl_pin;      // GPIO pin for the I²C clock line.
    uint8_t device_address;    // The 7-bit I²C address of the EEPROM.

    /**
     * @brief Performs any additional initialization for the EEPROM.
     *
     * This method can be extended to implement any further configuration required by the EEPROM.
     */
    void init();
};

#endif // EEPROM_STORAGE_H