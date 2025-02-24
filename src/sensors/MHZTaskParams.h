// MHZTaskParams.h or in the same file:
#pragma once

#include <memory>
#include "uart/PicoOsUart.hpp"

struct MHZTaskParams {
    std::shared_ptr<Uart::PicoOsUart> sensorUart;  // For MHZ19C sensor comms
    std::shared_ptr<Uart::PicoOsUart> logUart;     // For logging/debug output
};
