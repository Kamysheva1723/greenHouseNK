#include "FanDriver.h"
#include "ModbusRegister.h"
#include <cstdio>
#include <algorithm> // For std::clamp in C++17; if not available, a custom clamp function is provided below.

/*
   Custom clamp function in case std::clamp is not available.
   It constrains the value 'val' to be within the range [lo, hi].
*/
static float clamp(float val, float lo, float hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

/*
   FanDriver Class Constructor

   This constructor accepts a shared pointer to a ModbusRegister, which is used
   to communicate with the fan driver module over a Modbus RTU interface.
   It initializes the current fan speed to 0%.
*/
FanDriver::FanDriver(std::shared_ptr<ModbusRegister> fanRegister)
        : fanReg_(fanRegister),
          currentSpeed_(0.0f)
{
}

/*
   setFanSpeed()

   This method sets the operating speed of the fan based on the control logic's input.
   It accepts a percentage value between 0 and 100, clamps the value to the valid range,
   and then scales it to a corresponding raw value expected by the fan hardware (0 to 1000).
   The scaled value is then written to the Modbus register associated with the fan.

   @param percent The desired fan speed as a percentage (0 to 100).
*/
void FanDriver::setFanSpeed(float percent) {
    // Debug print: print the input percentage value.
    printf("percent: %f\n", percent);

    // Clamp the percentage to ensure it stays within the valid range [0, 100].
    percent = clamp(percent, 0.0f, 100.0f);
    // Uncomment the following line to force the fan speed to 0% if needed for testing.
    // percent = 0;

    // Convert the percentage to a raw 16-bit scale value (0 to 1000).
    // This conversion is necessary because the fan driver hardware might expect a value in this range.
    uint16_t rawValue = static_cast<uint16_t>((percent / 100.0f) * 1000.0f);

    // Write the calculated raw value to the Modbus register controlling the fan speed.
    // The ModbusRegister class abstracts the low-level register communication.
    fanReg_->write(rawValue);

    // Optionally: Check for write errors by modifying ModbusRegister::write() to return a status.
    // Example (commented out):
    // if (!fanReg_->write(rawValue)) {
    //     printf("FanDriver: write register failed\n");
    // }

    // Update the current fan speed stored for status reporting.
    currentSpeed_ = percent;
}

/*
   getCurrentSpeed()

   Returns the last commanded fan speed as a percentage. This is useful for status reporting
   and ensuring that the system has a record of the most recent control command sent to the fan.
   
   @return The current fan speed percentage.
*/
float FanDriver::getCurrentSpeed() const {
    return currentSpeed_;
}