#include "FanDriver.h"
#include "ModbusRegister.h"
#include <cstdio>
#include <algorithm> // for std::clamp in C++17, or write your own clamp

static float clamp(float val, float lo, float hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

FanDriver::FanDriver(std::shared_ptr<ModbusRegister> fanRegister)
        : fanReg_(fanRegister),
          currentSpeed_(0.0f)
{
}

void FanDriver::setFanSpeed(float percent) {
    printf("percent:",percent);
    // Limit to [0..100]
    percent = clamp(percent, 0.0f, 100.0f);
    //percent=0;

    // Convert [0..100] to a 16-bit [0..1000] scale,
    // if your device expects 0..1000 for 0..100%:
    uint16_t rawValue = static_cast<uint16_t>((percent / 100.0f) * 1000.0f);

    // Write to the register
    fanReg_->write(rawValue);

    if (false) {
        // If need to check for errors - modify ModbusRegister::write()
        // to return a boolean or NMBS error code, then:
        // if (!fanReg_->write(rawValue)) {
        //     printf("FanDriver: write register failed\n");
        // }
    }

    currentSpeed_ = percent;
}

float FanDriver::getCurrentSpeed() const {
    return currentSpeed_;
}
