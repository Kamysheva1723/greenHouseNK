//
// Created by natal on 26.1.2025.
//

#ifndef GREENHOUSE_EEPROM_H
#define GREENHOUSE_EEPROM_H

#include <cstdint>
#include <string>
#include <memory>

class PicoI2C;

/**
 * @brief Class for reading/writing configuration data to I2C-based EEPROM.
 */
class EEPROMStorage {
public:
    EEPROMStorage(std::shared_ptr<PicoI2C> i2c, uint8_t i2cAddress);

    // Example usage:
    bool storeCO2Setpoint(uint16_t co2ppm);
    uint16_t loadCO2Setpoint();

    bool storeNetworkCredentials(const std::string &ssid, const std::string &password);
    bool loadNetworkCredentials(std::string &ssid, std::string &password);

    // Additional methods as needed, e.g. store API key, store user config, etc.
private:
    std::shared_ptr<PicoI2C> i2c_;
    uint8_t address_;

    // methods to read/write raw bytes, plus offset definitions
    bool writeBytes(uint16_t address, const uint8_t *data, size_t length);
    bool readBytes(uint16_t address, uint8_t *data, size_t length);
};

#endif //GREENHOUSE_EEPROM_H
