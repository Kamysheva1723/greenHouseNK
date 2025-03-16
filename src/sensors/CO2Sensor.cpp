#include "CO2Sensor.h"              // Include header file for CO2Sensor class definition
#include <cstdio>                   // Provides printf() for formatted output
#include <cstdint>                  // Defines fixed-width integer types (e.g., uint16_t, uint32_t)
#include <cstring>                  // Provides functions like memcpy() for memory manipulation
#include "ModbusRegister.h"         // Include header for ModbusRegister class to handle Modbus register I/O operations

//--------------------------------------------------------------------------------------
// Helper Functions
//--------------------------------------------------------------------------------------

// Function to swap the bytes of a 16-bit unsigned integer.
// This may be necessary when converting between little-endian and big-endian formats
// especially if the Modbus library doesn't automatically handle the conversion.
static inline uint16_t swapBytes(uint16_t val) {
    // Shift the high byte to the low position and vice versa, then combine them.
    return (val >> 8) | (val << 8);
}

// A simple delay function based on FreeRTOS's vTaskDelay.
// Converts the given milliseconds to OS ticks using pdMS_TO_TICKS macro.
static inline void shortDelayMs(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

//--------------------------------------------------------------------------------------
// CO2Sensor Class Implementation
//--------------------------------------------------------------------------------------

// Constructor for the CO2Sensor class.
// Initializes the sensor object with shared pointers to Modbus registers associated with:
// 1) the raw CO2 measurement,
// 2) the overall device status, and
// 3) the CO2-specific status register.
CO2Sensor::CO2Sensor(std::shared_ptr<ModbusRegister> co2LowReg,
                     std::shared_ptr<ModbusRegister> deviceStatusReg,
                     std::shared_ptr<ModbusRegister> co2StatusReg)
        : co2LowReg_(co2LowReg),              // Initialize pointer to the CO2 low reading register
          deviceStatusReg_(deviceStatusReg),  // Initialize pointer to the device status register
          co2StatusReg_(co2StatusReg),        // Initialize pointer to the CO2 status register
          co2ppm_(0.0f)                       // Set initial CO2 concentration (ppm) to zero
{
    // No additional initialization is required in the constructor body.
}

//--------------------------------------------------------------------------------------
// readSensor() Function
//--------------------------------------------------------------------------------------

// Function to read a CO2 sensor value.
// The function attempts to read the sensor up to a maximum number of tries (maxAttempts).
// For each attempt, it checks both the device and CO2-specific status registers:
// - A reading is only accepted if both registers return 0 (indicating a valid, error-free condition).
// - The function always reads the raw register to clear the UART buffer, even if the reading is invalid.
// If a valid reading is obtained, it updates the internal CO2 ppm value and returns true.
// If no valid reading is obtained after maxAttempts, the function returns false.
bool CO2Sensor::readSensor() {
    const int maxAttempts = 10;    // Maximum number of attempts to obtain a valid sensor reading
    bool success = false;          // Flag to indicate if a valid sensor reading was achieved
    float newReading = 0.0f;       // Temporary variable to hold the new CO2 reading

    // Loop through a fixed number of attempts to get a valid sensor reading
    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        // 1) Check device status register; a return value of 0 indicates the sensor hardware is OK.
        bool devOK = true;
        if (deviceStatusReg_) {  // Ensure the device status register pointer is valid
            uint16_t devStatus = deviceStatusReg_->read();  // Read the device status from Modbus register
            std::printf("[CO2Sensor] Device Status = 0x%04X\n", devStatus);
            if (devStatus != 0) {  // A non-zero status signals an error with the sensor device
                std::printf("[CO2Sensor] WARNING: device status not OK (0x%04X), attempt %d\n",
                            devStatus, attempt + 1);
                devOK = false;
            }
        }

        // 2) Check CO2-specific status register; a status of 0 means CO2 measurement is valid.
        bool co2OK = true;
        if (co2StatusReg_) {  // Validate the pointer to the CO2 status register
            uint16_t status = co2StatusReg_->read();  // Read the CO2 status register value
            std::printf("[CO2Sensor] CO₂ Status = 0x%04X\n", status);
            if (status != 0) {  // A non-zero value here indicates an issue with the CO2 reading
                std::printf("[CO2Sensor] WARNING: CO₂ status not OK (0x%04X), attempt %d\n",
                            status, attempt + 1);
                co2OK = false;
            }
        }

        // 3) Read the raw value from the CO2 low register regardless of status,
        //    ensuring that any pending data in the UART buffer is cleared.
        if (!co2LowReg_) {  // Safety check to ensure the CO2 low register pointer is not null
            std::printf("[CO2Sensor] ERROR: Null register pointer!\n");
            return false;
        }
        uint16_t raw = co2LowReg_->read();  // Obtain raw sensor reading from the Modbus register
        std::printf("[CO2Sensor] Raw=0x%04X => ~%.1f ppm (tentative)\n", raw, (float)raw);

        // 4) If both status registers indicate OK conditions, accept the reading.
        if (devOK && co2OK) {
            newReading = static_cast<float>(raw);  // Convert raw integer value to a floating-point ppm value
            std::printf("[CO2Sensor] Accepting new CO₂ reading: %.1f ppm\n", newReading);
            success = true;  // Mark read as successful
            break;           // Exit the retry loop as a valid reading has been obtained
        }

        // If the reading was not valid, wait for a short delay before retrying.
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // After exiting the loop, update the stored CO2 ppm value if a valid reading was acquired.
    if (success) {
        co2ppm_ = newReading;
    } else {
        // If no valid reading was achieved after maxAttempts, log an error and return failure.
        std::printf("[CO2Sensor] ERROR: No valid reading after %d attempts.\n", maxAttempts);
        return false;
    }

    // Optional extra delay after a successful reading.
    // This delay may allow the sensor some settling time before the next reading.
    vTaskDelay(pdMS_TO_TICKS(3000));
    return true;  // Return true indicating that the sensor reading was successfully updated
}

//--------------------------------------------------------------------------------------
// Accessor Functions
//--------------------------------------------------------------------------------------

// Getter function to retrieve the most recently obtained CO2 concentration in parts per million (ppm).
float CO2Sensor::getCO2ppm() const {
    return co2ppm_;
}

// Another accessor function that simply acts as a wrapper for getCO2ppm().
// Provides a uniform interface for obtaining the sensor's value.
float CO2Sensor::getValue() {
    return getCO2ppm();
}