#pragma once

#include <memory>
#include <cstdint>

class ModbusRegister;

/**
 * A simple fan driver that writes a 0..1000 value (0-100% speed)
 * to a specific Modbus holding register.
 */
class FanDriver {
public:
    /**
     * @param fanRegister  The ModbusRegister object pointing
     *                     to the holding register for fan speed.
     */
    FanDriver(std::shared_ptr<ModbusRegister> fanRegister);

    /**
     * Sets the fan speed in percent [0..100].
     */
    void setFanSpeed(float percent);

    /**
     * Returns the last set fan speed in percent.
     */
    float getCurrentSpeed() const;

private:
    std::shared_ptr<ModbusRegister> fanReg_;  ///< The Modbus register for fan speed
    float currentSpeed_;                      ///< Cached last speed in percent
};

