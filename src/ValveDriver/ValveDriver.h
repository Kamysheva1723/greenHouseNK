#ifndef VALVE_DRIVER_H
#define VALVE_DRIVER_H

#include <cstdint>   // Provides fixed-width integer types for consistency across platforms
#include <cstdio>    // Standard I/O library for debugging messages (if needed)

/**
 * @brief The ValveDriver class provides an abstraction for controlling a CO₂ valve using 
 *        a digital output via a GPIO pin.
 *
 * The class encapsulates basic operations including opening and closing the valve, 
 * as well as checking its current state. This is part of the actuator modules in the
 * Greenhouse Fertilization System, interfacing with hardware through a designated GPIO pin.
 * The driver is designed for quick and reliable response, ensuring that the actuator state
 * is updated and tracked consistently.
 */
class ValveDriver {
public:
    /**
     * @brief Constructor for ValveDriver.
     *
     * Initializes the specified GPIO pin as an output for controlling the valve, and ensures 
     * that the valve starts in a closed state.
     *
     * @param gpioPin The GPIO pin number used to control the CO₂ valve.
     */
    ValveDriver(uint gpioPin);
    
    /**
     * @brief Opens the CO₂ valve.
     *
     * This method sets the digital output of the specified GPIO pin to high, which activates 
     * the valve mechanism. It also updates the internal state flag to indicate that the valve
     * is currently open.
     */
    void openValve();
    
    /**
     * @brief Closes the CO₂ valve.
     *
     * This method sets the digital output of the specified GPIO pin to low, which deactivates 
     * the valve mechanism. It also updates the internal state flag to indicate that the valve 
     * is currently closed.
     */
    void closeValve();
    
    /**
     * @brief Checks if the valve is open.
     *
     * Provides a way to query the current state of the valve.
     *
     * @return True if the valve is open, false otherwise.
     */
    bool isOpen() const;
    
private:
    uint pin;       ///< GPIO pin number used for controlling the valve.
    bool valveOpen; ///< Internal flag tracking the current state of the valve (true if open, false if closed).
};

#endif // VALVE_DRIVER_H