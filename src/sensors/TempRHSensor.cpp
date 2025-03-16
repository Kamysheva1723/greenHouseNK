#include "TempRHSensor.h"            // Include header for the TempRHSensor class definition
#include "ModbusRegister.h"           // Include header for the ModbusRegister class, used for reading sensor registers
#include <cstdio>                     // Provides standard input/output functions (printf) for debugging/logging
#include <utility>                    // Provides std::move for efficient transfer of ownership in constructors

/*
 * Constructor for the TempRHSensor class.
 *
 * This constructor initializes the sensor readings to default values (0.0f) and 
 * accepts shared pointers to three ModbusRegister objects:
 *    - tempReg: Register containing the raw temperature data.
 *    - rhReg: Register containing the raw relative humidity data.
 *    - trhErrorReg: Register containing an error/status value for both temperature and humidity.
 *
 * The use of std::move ensures that the passed shared pointers are transferred efficiently 
 * to the member variables, preserving resource ownership.
 */
TempRHSensor::TempRHSensor(std::shared_ptr<ModbusRegister> tempReg, 
                           std::shared_ptr<ModbusRegister> rhReg, 
                           std::shared_ptr<ModbusRegister> trhErrorReg)
        : temperature(0.0f), humidity(0.0f), 
          treg(std::move(tempReg)), 
          rhreg(std::move(rhReg)), 
          trh_error_reg(std::move(trhErrorReg))
{
    // Empty constructor body since initialization is handled in the initializer list.
}

/*
 * readSensor - Reads and processes temperature and relative humidity data from the sensor.
 *
 * This method attempts to obtain valid sensor data by performing up to MAX_ATTEMPTS
 * (here, 5 attempts) until the sensor's error/status register indicates that data is valid.
 * 
 * The steps are:
 *   1. Check that all required Modbus registers (temperature, humidity, error/status) are valid.
 *   2. For each attempt:
 *        a) Read the error/status register.
 *        b) Read raw temperature and humidity values regardless.
 *        c) Print the read values for diagnostics.
 *        d) If the error/status indicates "OK" (value equals 1), then convert raw readings:
 *           - Scale raw temperature by dividing by 10 to get degrees Celsius.
 *           - Scale raw humidity by dividing by 10 to get relative humidity percentage.
 *        e) Log the accepted values and break out of the loop if successful.
 *        f) If the reading is not valid, delay for a short period (200 ms) before retrying.
 *   3. If no valid reading is obtained after all attempts, log an error and keep previous values.
 *
 * @return true if valid sensor data was successfully read and processed; false otherwise.
 */
bool TempRHSensor::readSensor() {
    // Check if the temperature, humidity, and error registers are available
    if (!treg || !rhreg || !trh_error_reg) {
        std::printf("[TRHSensor] ERROR: Invalid registers.\n");
        return false;
    }

    // Define constant for maximum number of attempts to read valid sensor data
    constexpr int MAX_ATTEMPTS = 5;
    bool success = false;  // Flag to track if a valid reading has been obtained

    // Attempt to read sensor data multiple times if necessary
    for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
        // 1) Read the error/status register to check sensor status
        uint16_t status = trh_error_reg->read();
        std::printf("[TRHSensor] Status = 0x%04X (attempt %d)\n", status, attempt + 1);

        // 2) Read raw temperature and humidity values regardless of error status.
        // These are tentative readings that will only be used if the status is valid.
        int raw_temp = treg->read();
        int raw_humidity = rhreg->read();
        std::printf("[TRHSensor] Raw T=%d, Raw RH=%d (tentative)\n", raw_temp, raw_humidity);

        // 3) Check if the sensor's error/status register indicates that the readings are valid.
        // A status value of 1 is interpreted as "OK" according to sensor specifications.
        if (status == 1) {
            // Convert raw temperature to degrees Celsius and humidity to percentage
            temperature = static_cast<float>(raw_temp) / 10.0f;
            humidity    = static_cast<float>(raw_humidity) / 10.0f;

            // Log the accepted values
            std::printf("[TRHSensor] Accepting new values: Temp=%.2f, RH=%.2f\n", temperature, humidity);
            success = true;
            break;  // Exit the loop as valid data has been obtained
        } else {
            // Sensor did not report valid data; log a warning and retry
            std::printf("[TRHSensor] WARNING: Status not OK (0x%04X). Retrying...\n", status);
        }

        // 4) Delay briefly (200 ms) before the next reading attempt to allow sensor stabilization
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    // If all attempts failed, log an error and retain previous sensor values (if any)
    if (!success) {
        std::printf("[TRHSensor] ERROR: No valid reading after %d attempts; keeping old values: Temp=%.2f, RH=%.2f\n",
                    MAX_ATTEMPTS, temperature, humidity);
    }

    return success;  // Return whether a valid reading was obtained
}

/*
 * getTemperature - Getter method for the temperature.
 *
 * This method returns the most recently accepted temperature value in degrees Celsius.
 *
 * @return Current temperature in °C.
 */
float TempRHSensor::getTemperature() const {
    return temperature;
}

/*
 * getHumidity - Getter method for the relative humidity.
 *
 * This method returns the most recently accepted relative humidity value in percentage.
 *
 * @return Current relative humidity in %.
 */
float TempRHSensor::getHumidity() const {
    return humidity;
}