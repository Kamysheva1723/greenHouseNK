#include "ValveDriver.h"
#include "hardware/gpio.h"

ValveDriver::ValveDriver(uint gpioPin)
        : pin(gpioPin), valveOpen(false)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, false);
}

void ValveDriver::openValve() {
    gpio_put(pin, true);
    valveOpen = true;
}

void ValveDriver::closeValve() {
    gpio_put(pin, false);
    valveOpen = false;
}

bool ValveDriver::isOpen() const {
    return valveOpen;
}
