//
// Sensors.cpp
//
// This source file is part of the Greenhouse Fertilization System firmware.
// It provides the foundation for the sensor modules which implement the ISensor interface.
// These sensors include CO₂, temperature, humidity, and pressure sensors.
// The file leverages hardware abstraction for both I²C and Modbus RTU protocols to communicate with the sensors.

#include "ISensor.h"     // Include the common sensor interface to ensure all sensors provide a unified API.
#include <cstdio>        // Provides standard I/O functions, such as printf, for debugging and status logging.
#include <utility>       // Provides utility functions and operations (e.g., std::move) useful for resource management.
#include "ModbusRegister.h"  // Provides access to Modbus registers; essential for sensors communicating via Modbus RTU (e.g., CO₂ sensor).
#include "PicoI2C.h"         // Provides the PicoI2C driver, which abstracts I²C communication used by sensors like the pressure sensor, SSD1306 display, and EEPROM.