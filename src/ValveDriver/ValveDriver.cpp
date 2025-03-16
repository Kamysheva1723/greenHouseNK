#include "ValveDriver.h"          // Include the header file for the ValveDriver class
#include "hardware/gpio.h"          // Include the Pico SDK's GPIO header for digital I/O operations

/*
 * Constructor for the ValveDriver class.
 *
 * This constructor initializes the GPIO pin provided as a digital output to control a CO₂ valve.
 * The valve is initially set to the "closed" state (GPIO low).
 *
 * @param gpioPin: The GPIO pin number that is connected to the valve driver.
 */
ValveDriver::ValveDriver(uint gpioPin)
        : pin(gpioPin), valveOpen(false)  // Initialize the pin number and set the valve to closed (false)
{
    gpio_init(pin);                // Initialize the specified GPIO pin
    gpio_set_dir(pin, GPIO_OUT);   // Set the GPIO pin to output mode for controlling the valve
    gpio_put(pin, false);          // Ensure the valve is initially closed by setting the output low
}

/*
 * openValve - Opens the CO₂ valve.
 *
 * This function sets the output of the configured GPIO pin high, which activates the valve,
 * and updates the internal status flag to indicate that the valve is open.
 */
void ValveDriver::openValve() {
    gpio_put(pin, true);           // Set the GPIO pin high to open the valve
    valveOpen = true;              // Update internal state to reflect that the valve is open
}

/*
 * closeValve - Closes the CO₂ valve.
 *
 * This function sets the output of the configured GPIO pin low, which deactivates the valve,
 * and updates the internal status flag to indicate that the valve is closed.
 */
void ValveDriver::closeValve() {
    gpio_put(pin, false);          // Set the GPIO pin low to close the valve
    valveOpen = false;             // Update internal state to reflect that the valve is closed
}

/*
 * isOpen - Returns the current state of the CO₂ valve.
 *
 * This function provides a simple query to check whether the valve is currently open.
 *
 * @return True if the valve is open, false otherwise.
 */
bool ValveDriver::isOpen() const {
    return valveOpen;             // Return the status flag indicating the valve state
}