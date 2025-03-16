#pragma once
// This header defines the PressureSensor class, which implements the ISensor interface.
// The PressureSensor is used to read differential pressure from a Sensirion SDP6xx/SDP610 sensor via I2C.
// It retrieves a signed 16-bit measurement that is converted into pressure in Pascals using a scaling factor.
// Additional functionalities include optional CRC checking and altitude-based pressure correction.

#include <memory>     // For std::shared_ptr
#include <cstdint>    // For fixed-width integer types such as uint8_t
#include "ISensor.h"  // Include the sensor interface, ensuring PressureSensor conforms to a common API

class PicoI2C; // Forward declaration of PicoI2C to enable use without requiring the full header here

/**
 * @brief The PressureSensor class reads differential pressure data from a Sensirion SDP sensor over I2C.
 *
 * The sensor returns a signed 16-bit reading which is scaled by a factor (typically 60 counts/Pa)
 * to convert the raw reading into pressure (Pascals). Optionally, the class can read a CRC byte to
 * validate the data and can apply altitude correction using an estimated ambient pressure model.
 */
class PressureSensor : public ISensor {
public:
    /**
     * @brief Constructs a PressureSensor object.
     *
     * This constructor initializes the PressureSensor with the given I2C bus instance, sensor address,
     * altitude (for pressure correction), and whether to enable CRC checking.
     *
     * Default arguments are provided for convenience:
     *   - i2cAddress defaults to 0x40, which is common for the SDP series.
     *   - altitude_m is 0.0f by default (i.e., sea level).
     *   - useCrc is false by default, meaning CRC checking is disabled unless explicitly enabled.
     *
     * @param my_i2c      Shared pointer to a PicoI2C instance representing the I2C bus.
     * @param i2cAddress  The 7-bit I2C address of the pressure sensor.
     * @param altitude_m  Altitude in meters for ambient pressure correction.
     * @param useCrc      Flag indicating if CRC checking of the sensor data should be performed.
     */
    PressureSensor(std::shared_ptr<PicoI2C> my_i2c,
                   uint8_t i2cAddress = 0x40,
                   float altitude_m = 0.0f,
                   bool useCrc = false);

    /**
     * @brief Reads the sensor data from the pressure sensor.
     *
     * This method communicates with the sensor over I2C. It sends a command to request a new pressure
     * measurement, then reads the response bytes. The raw data is then scaled (and corrected) to obtain
     * the pressure in Pascals. If CRC checking is enabled, it validates the data using the checkCrc8() function.
     *
     * @return True if the sensor data was read successfully and is valid, false otherwise.
     */
    bool readSensor();

    /**
     * @brief Retrieves the current pressure reading.
     *
     * This method implements the ISensor interface and returns the last valid pressure reading in Pascals.
     *
     * @return The pressure reading in Pascals.
     */
    float getValue();

    /**
     * @brief Retrieves the current pressure in Pascals.
     *
     * Provides direct access to the last stored pressure value.
     *
     * @return The previously measured pressure value in Pascals.
     */
    float getPressurePa() const;

private:
    std::shared_ptr<PicoI2C> my_i2c;    ///< I2C bus interface used for sensor communication
    uint8_t address;                   ///< 7-bit I2C address of the pressure sensor
    float pressurePa;                  ///< Last measured differential pressure in Pascals
    float altitudeMeters;              ///< Altitude (in meters) used for pressure correction calculations
    bool crcEnabled;                   ///< Flag indicating if CRC verification is enabled

    /**
     * @brief Approximates the ambient pressure based on altitude.
     *
     * Utilizes a simple barometric formula or lookup (implementation-dependent) to estimate the
     * ambient pressure at a given altitude (in meters). This value can be used for altitude correction.
     *
     * @param altitude_m Altitude in meters for which ambient pressure should be estimated.
     * @return Estimated ambient pressure in millibars (mbar).
     */
    float approximateAmbientPressure(float altitude_m) const;

    /**
     * @brief Validates the sensor data using an 8-bit CRC.
     *
     * This function calculates the CRC for the provided two data bytes and compares it to the sensor's
     * transmitted CRC byte. It is used only if CRC checking is enabled.
     *
     * @param msb     The most significant byte of the sensor data.
     * @param lsb     The least significant byte of the sensor data.
     * @param crcByte The CRC byte received from the sensor.
     * @return True if the calculated CRC matches the provided CRC byte; false otherwise.
     */
    bool checkCrc8(uint8_t msb, uint8_t lsb, uint8_t crcByte);
};