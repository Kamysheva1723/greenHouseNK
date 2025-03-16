#ifndef ISENSOR_H
#define ISENSOR_H

/*
 * ISensor Interface
 *
 * This interface is part of the Greenhouse Fertilization System firmware.
 * It defines a common API for all sensor modules (e.g., CO₂ sensor, Temperature & Humidity sensor, Pressure sensor)
 * used in the system. All sensor modules must implement this interface to facilitate a modular and
 * uniform approach for sensor data acquisition, which is integrated into the system's FreeRTOS-based multitasking.
 *
 * Implementations of this interface are responsible for:
 *   - Reading raw sensor data via communication protocols (e.g., I²C, Modbus RTU).
 *   - Performing any necessary calibration and error checking before producing a valid output.
 *
 * The interface is designed with the principle of separation of concerns: each sensor module
 * encapsulates the logic specific to that sensor type, allowing higher-level software components,
 * such as sensorTask or Controller, to access sensor data through a common set of methods.
 */
class ISensor {
public:
    /**
     * @brief Reads data from the sensor.
     *
     * Each sensor implementation should perform the following steps on invocation:
     *   - Initiate a hardware communication read (e.g., via I²C or Modbus RTU).
     *   - Validate the received data via calibration and error-checking procedures.
     *   - Update its internal state with the latest valid sensor reading.
     *
     * This interface method ensures asynchronous and periodic sensor data acquisition,
     * facilitating real-time control loops as outlined in the system design.
     *
     * @return true if the sensor reading was successfully obtained and is valid, false otherwise.
     */
    virtual bool readSensor() = 0;
    
    // Virtual destructor allows for proper cleanup in derived classes.
    virtual ~ISensor() = default;
};

#endif // ISENSOR_H