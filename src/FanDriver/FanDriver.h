//
// Created by natal on 26.1.2025.
//

#ifndef GREENHOUSE_FUNDRIVER_H
#define GREENHOUSE_FUNDRIVER_H

#include <cstdint>
#include <memory>

class PicoOsUart;

/**
 * @brief Interface to control fan speed (0–100%) via MIO 12‐V over Modbus.
 */
class FanDriver {
public:
    FanDriver(std::shared_ptr<PicoOsUart> uart, uint8_t modbusAddress);
    ~FanDriver() = default;

    /**
     * @brief Set the fan speed from 0–100%.
     */
    void setFanSpeed(float percent);

    /**
     * @brief Optional: read back fan pulse count or speed if needed.
     */
    float getFanSpeed() const { return currentSpeed_; }

private:
    std::shared_ptr<PicoOsUart> uart_;
    uint8_t address_;
    float currentSpeed_; // 0–100
};

#endif //GREENHOUSE_FUNDRIVER_H
