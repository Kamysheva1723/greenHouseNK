#pragma once

#include <memory>
#include <cstdint>
#include "ISensor.h"

class PicoI2C;

/**
 * @brief A class to read differential pressure from a Sensirion SDP6xx/SDP610 sensor via I2C.
 *
 *        The sensor reading is a signed 16-bit integer, scaled by a factor (commonly 60 counts/Pa).
 *        This class also optionally supports reading a CRC byte, and applying altitude correction.
 */
// PressureSensor inherits from ISensor:
class PressureSensor : public ISensor {
public:
    // Default args ONLY here, in the header
    PressureSensor(std::shared_ptr<PicoI2C> my_i2c,
                   uint8_t i2cAddress = 0x40,
                   float altitude_m = 0.0f,
                   bool useCrc = false);

    bool readSensor();
    float getValue();

    float getPressurePa() const;

private:
    std::shared_ptr<PicoI2C> my_i2c;   ///< The I2C bus interface
    uint8_t address;                  ///< The 7-bit I2C address of the sensor
    float pressurePa;                 ///< Last read differential pressure, in Pascals
    float altitudeMeters;             ///< Altitude for correction
    bool crcEnabled;                  ///< Whether to read/check the CRC byte

    /**
     * @brief Approximates ambient pressure for a given altitude (meters above sea level).
     *
     * @param altitude_m   Altitude in meters
     * @return Estimated ambient pressure in millibars (mbar)
     */
    float approximateAmbientPressure(float altitude_m) const;

    /**
     * @brief Checks an 8-bit CRC for the two data bytes.
     *
     * @param msb    High data byte
     * @param lsb    Low data byte
     * @param crcByte The CRC byte from the sensor
     * @return True if CRC is valid, false otherwise.
     */
    bool checkCrc8(uint8_t msb, uint8_t lsb, uint8_t crcByte);
};
