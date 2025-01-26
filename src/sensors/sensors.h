//
// Created by natal on 26.1.2025.
//

#ifndef GREENHOUSE_SENSORS_H
#define GREENHOUSE_SENSORS_H
#include <cstdint>
#include <memory>

// Forward declaration of hardware/transport classes, e.g. PicoOsUart, PicoI2C, etc.
class PicoOsUart;
class PicoI2C;

/**
 * @brief Abstract base for a sensor that can be polled to get updated readings.
 */
class ISensor {
public:
    virtual ~ISensor() {}
    virtual bool readSensor() = 0;  // main read/poll operation
};

/**
 * @brief CO2 sensor using Modbus RTU (GMP252).
 */
class CO2Sensor : public ISensor {
public:
    CO2Sensor(std::shared_ptr<PicoOsUart> uart, uint8_t modbusAddress);
    bool readSensor() override;

    float getCO2ppm() const { return co2ppm_; }

private:
    std::shared_ptr<PicoOsUart> uart_;
    uint8_t address_;
    float co2ppm_;
};

/**
 * @brief Combined Temperature & RH sensor using Modbus RTU (HMP60).
 */
class TempRHSensor : public ISensor {
public:
    TempRHSensor(std::shared_ptr<PicoOsUart> uart, uint8_t modbusAddress);
    bool readSensor() override;

    float getTemperature() const { return temperature_; }
    float getHumidity() const    { return humidity_; }

private:
    std::shared_ptr<PicoOsUart> uart_;
    uint8_t address_;
    float temperature_;
    float humidity_;
};

/**
 * @brief Differential pressure sensor (SDP610) using I2C.
 */
class PressureSensor : public ISensor {
public:
    PressureSensor(std::shared_ptr<PicoI2C> i2c, uint8_t i2cAddress);
    bool readSensor() override;

    float getPressurePa() const { return pressurePa_; }

private:
    std::shared_ptr<PicoI2C> i2c_;
    uint8_t address_;
    float pressurePa_;
};
#endif //GREENHOUSE_SENSORS_H
