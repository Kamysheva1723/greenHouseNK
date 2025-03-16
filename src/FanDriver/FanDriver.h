#pragma once

#include <memory>
#include <cstdint>

class ModbusRegister;

/*
   FanDriver Class

   This class provides a high-level interface to control a fan actuator in the Greenhouse Fertilization System.
   It writes a raw value (ranging from 0 to 1000) to a specific Modbus holding register which corresponds to 0-100% fan speed.
   The class abstracts the scaling and clamping necessary for safe and reliable fan operation.
*/
class FanDriver {
public:
    /**
     * @brief Constructor for FanDriver.
     *
     * Initializes the FanDriver with a pointer to the ModbusRegister that is used to control the fan.
     * The provided ModbusRegister object encapsulates the low-level communication details over Modbus RTU.
     *
     * @param fanRegister A shared pointer representing the register address used for fan speed control.
     */
    FanDriver(std::shared_ptr<ModbusRegister> fanRegister);

    /**
     * @brief Sets the fan speed.
     *
     * This method takes a fan speed percentage (0% - 100%), clamps the value within that range,
     * converts it to a raw value on a scale of 0 to 1000 (as expected by the fan hardware), 
     * and then writes the raw value into the corresponding Modbus register.
     * The current speed is cached for later retrieval.
     *
     * @param percent The desired fan speed as a percentage (0.0 to 100.0).
     */
    void setFanSpeed(float percent);

    /**
     * @brief Retrieves the last set fan speed.
     *
     * Returns the fan speed that was last commanded, expressed as a percentage,
     * which is useful for status reporting and verifying control actions.
     *
     * @return The current fan speed in percent.
     */
    float getCurrentSpeed() const;

private:
    std::shared_ptr<ModbusRegister> fanReg_;  ///< Pointer to the Modbus register for fan speed control.
    float currentSpeed_;                      ///< Cached fan speed (in percent) as last commanded.
};