#ifndef VALVE_DRIVER_H
#define VALVE_DRIVER_H

#include <cstdint>
#include <cstdio>

class ValveDriver {
public:
    ValveDriver(uint gpioPin);
    void openValve();
    void closeValve();
    bool isOpen() const;
private:
    uint pin;
    bool valveOpen;
};

#endif // VALVE_DRIVER_H
