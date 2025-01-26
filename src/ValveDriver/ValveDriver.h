//
// Created by natal on 26.1.2025.
//

#ifndef GREENHOUSE_VALVEDRIVER_H
#define GREENHOUSE_VALVEDRIVER_H

#include <cstdint>

/**
 * @brief Simple class controlling the solenoid valve (CO2 injection) via a GPIO pin.
 */
class ValveDriver {
public:
    ValveDriver(uint gpioPin);
    ~ValveDriver() = default;

    void openValve();
    void closeValve();

    bool isOpen() const { return valveOpen_; }

private:
    uint pin_;
    bool valveOpen_;
};

#endif //GREENHOUSE_VALVEDRIVER_H
