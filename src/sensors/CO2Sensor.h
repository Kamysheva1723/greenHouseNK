#ifndef CO2SENSOR_H
#define CO2SENSOR_H

#include <memory>       // Provides smart pointers (e.g., std::shared_ptr) for dynamic object management.
#include <cstdint>      // Defines fixed-width integer types (e.g., uint16_t) for predictable behavior on microcontrollers.
#include "ISensor.h"    // Include the sensor interface, establishing a common API for all sensor modules in the system.

/*
 * GreenHouse Fertilization System - CO2Sensor Module
 *
 * This module implements the ISensor interface for reading CO₂ concentration in parts per million (ppm)
 * from a GMP252-like sensor using Modbus RTU communication. It abstracts the low-level Modbus register access,
 * handling sensor calibration and error checking according to the system's design specifications.
 *
 * Key features based on the design documentation include:
 *   - Repeated attempts (up to 10) to obtain a valid CO₂ reading.
 *   - Verification of both overall device status and CO₂-specific status using dedicated Modbus registers.
 *   - Exposure of the latest valid sensor reading via a unified interface for the sensorTask.
 *   - Integration into the FreeRTOS based modular system, enabling reliable real-time sensor data acquisition.
 */

/**
 * @brief The CO2Sensor class implements the ISensor interface for reading CO₂ (ppm)
 *        from a GMP252-like sensor using Modbus registers.
 *
 * This class encapsulates the functionality for reading the raw CO₂ measurement,
 * verifying sensor operational statuses, and providing the most recent valid CO₂ reading.
 * It is designed to be integrated with the overall modular system architecture of the
 * Greenhouse Fertilization System, supporting real-time control and monitoring.
 */
class CO2Sensor : public ISensor {
public:
    /**
     * @brief Constructs a CO2Sensor object.
     *
     * In accordance with the system's modular hardware abstraction design, this constructor
     * receives shared pointers to three Modbus registers:
     *   - co2LowReg: Holds the raw 16-bit measurement value representing the CO₂ concentration.
     *   - deviceStatusReg: Monitors the general status of the sensor device (e.g., error codes).
     *   - co2StatusReg: Specifically indicates the status of the CO₂ measurement.
     *
     * These registers are typically instantiated and provided by the system initialization module
     * during the FreeRTOS setupTask, ensuring proper dynamic resource allocation.
     *
     * @param co2LowReg Shared pointer to the ModbusRegister for the CO₂ measurement.
     * @param deviceStatusReg Shared pointer to the ModbusRegister for the overall device status.
     * @param co2StatusReg Shared pointer to the ModbusRegister for the CO₂-specific sensor status.
     */
    CO2Sensor(std::shared_ptr<ModbusRegister> co2LowReg,
              std::shared_ptr<ModbusRegister> deviceStatusReg,
              std::shared_ptr<ModbusRegister> co2StatusReg);

    /**
     * @brief Reads the CO₂ sensor measurement.
     *
     * This method implements the sensor calibration and error-checking strategy as described in the system documentation.
     * It attempts to read a valid sensor value by performing up to 10 attempts. On each attempt, it:
     *   - Checks the overall device status from the deviceStatusReg (expecting a 0 to confirm normal operation).
     *   - Checks the CO₂-specific status from the co2StatusReg (again, expecting a 0 to indicate valid reading).
     *   - Reads the raw value from the co2LowReg to clear any pending data in the UART buffer.
     *
     * Only when both status registers indicate successful operation is the raw reading accepted and stored.
     * This approach minimizes transient errors and ensures that the control logic receives reliable data.
     *
     * @return true if a valid reading was obtained within the maximum attempt count; false otherwise.
     */
    bool readSensor();

    /**
     * @brief Retrieves the last measured CO₂ concentration in parts per million (ppm).
     *
     * This getter function provides access to the latest valid reading stored by the sensor.
     * It is used by the controller and UI modules to update system states and display accurate data.
     *
     * @return The most recent CO₂ reading in ppm.
     */
    float getCO2ppm() const;

    /**
     * @brief Returns the sensor's current reading.
     *
     * This method implements the generic ISensor interface and is an alias for getCO2ppm().
     * It allows the sensorTask to uniformly access sensor data from multiple sensor types.
     *
     * @return The current CO₂ concentration (in ppm).
     */
    float getValue();

private:
    // Pointer to the Modbus register that contains the raw 16-bit CO₂ measurement.
    std::shared_ptr<ModbusRegister> co2LowReg_;

    // Pointer to the Modbus register that provides the overall device status.
    // A value of 0 indicates that the sensor hardware is operating normally.
    std::shared_ptr<ModbusRegister> deviceStatusReg_;

    // Pointer to the Modbus register that provides the CO₂-specific sensor status.
    // A value of 0 indicates that the CO₂ measurement is valid and free from error.
    std::shared_ptr<ModbusRegister> co2StatusReg_;

    // Stores the most recent valid CO₂ measurement (in ppm) retrieved from the sensor.
    // This stored value is updated via readSensor() and accessed by both the Controller
    // and the UI modules for display and control purposes.
    float co2ppm_;
};

#endif  // CO2SENSOR_H