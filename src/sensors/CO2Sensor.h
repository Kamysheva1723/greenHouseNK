#ifndef CO2SENSOR_H
#define CO2SENSOR_H

#include <memory>
#include <cstdint>
#include "ISensor.h"

// Forward-declare ModbusRegister to avoid pulling in large headers.
class ModbusRegister;

/**
 * @brief CO2Sensor implements the ISensor interface for reading CO₂ (ppm)
 *        from a GMP252-like sensor.
 */
class CO2Sensor : public ISensor {
public:
    // The constructor now takes four ModbusRegister pointers:
    // - co2LowReg: for the 16 bit

    // - deviceStatusReg: for device status (from address 2048)
    // - co2StatusReg: for CO₂ status (from address 2049)
    CO2Sensor(std::shared_ptr<ModbusRegister> co2LowReg,

              std::shared_ptr<ModbusRegister> deviceStatusReg,
              std::shared_ptr<ModbusRegister> co2StatusReg);

    // Reads the sensor measurement; returns true if successful.
    bool readSensor();

    // Returns the last measured CO₂ concentration in ppm.
    float getCO2ppm() const;
    float getValue();

private:
    std::shared_ptr<ModbusRegister> co2LowReg_;

    std::shared_ptr<ModbusRegister> deviceStatusReg_;
    std::shared_ptr<ModbusRegister> co2StatusReg_;

    float co2ppm_;
};

#endif  // CO2SENSOR_H