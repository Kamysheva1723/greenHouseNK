#ifndef TEMP_RH_SENSOR_H
#define TEMP_RH_SENSOR_H

#include <memory>                 // For std::shared_ptr
#include "ISensor.h"              // For the ISensor interface

// Forward declaration to avoid including the full definition of the ModbusRegister class.
class ModbusRegister;

/**
 * @brief TempRHSensor class for reading temperature and relative humidity via Modbus registers.
 *
 * This sensor module implements the ISensor interface. It retrieves raw sensor data from three Modbus registers:
 * - One for temperature (tempReg).
 * - One for relative humidity (rhReg).
 * - One for the combined error/status (trhErrorReg) for both sensors.
 *
 * The raw temperature and humidity values are scaled (divided by 10) to convert them into meaningful units:
 * - Temperature in degrees Celsius (°C).
 * - Relative Humidity in percentage (%).
 *
 * The sensor reading process includes multiple attempts (up to 5) to ensure data validity as specified by the error/status register.
 */
class TempRHSensor : public ISensor {
public:
    /**
     * @brief Constructs a TempRHSensor instance.
     *
     * Initializes temperature and humidity values to 0.0 and sets up shared pointers
     * for accessing the Modbus registers used to read sensor data and error/status information.
     *
     * @param tempReg     Shared pointer to the ModbusRegister holding raw temperature data.
     * @param rhReg       Shared pointer to the ModbusRegister holding raw relative humidity data.
     * @param trhErrorReg Shared pointer to the ModbusRegister holding error/status data for calibration.
     */
    TempRHSensor(std::shared_ptr<ModbusRegister> tempReg,
                 std::shared_ptr<ModbusRegister> rhReg,
                 std::shared_ptr<ModbusRegister> trhErrorReg);

    /**
     * @brief Reads the temperature and relative humidity from the sensor.
     *
     * This method attempts to read valid sensor data by:
     *   1. Checking that all required registers are valid.
     *   2. Trying up to 5 times to obtain a valid reading by first reading the error/status register.
     *   3. Obtaining raw temperature and humidity data regardless of the error/status.
     *   4. If the error/status indicates "OK" (value equals 1), it scales and accepts the reading.
     *   5. A delay is applied between attempts to allow sensor stabilization.
     *
     * If a valid reading is not obtained after all attempts, it logs an error and retains previous values.
     *
     * @return true if a valid reading was obtained, false otherwise.
     */
    bool readSensor() override;

    /**
     * @brief Retrieves the current temperature value.
     *
     * @return The temperature in degrees Celsius.
     */
    float getTemperature() const;

    /**
     * @brief Retrieves the current relative humidity value.
     *
     * @return The relative humidity in percentage.
     */
    float getHumidity() const;
    
private:
    float temperature; ///< Stores the current temperature (in °C).
    float humidity;    ///< Stores the current relative humidity (in %).

    // Shared pointers to Modbus registers for sensor data and error/status checking.
    std::shared_ptr<ModbusRegister> treg;         ///< Register for raw temperature data.
    std::shared_ptr<ModbusRegister> rhreg;          ///< Register for raw relative humidity data.
    std::shared_ptr<ModbusRegister> trh_error_reg;  ///< Register for sensor error/status flags.
};

#endif // TEMP_RH_SENSOR_H