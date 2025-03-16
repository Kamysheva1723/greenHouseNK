#ifndef INIT_DATA_H
#define INIT_DATA_H

#include <memory>                        // For smart pointers like std::shared_ptr
#include "EEPROM/EEPROMStorage.h"        // Provides interface for non-volatile storage via external EEPROM
#include "./Controller/Controller.h"     // Defines the Controller class that manages sensor data and actuation logic
#include "UI/ui.h"                       // Defines the UI class that manages the on-device display and user interactions
#include <vector>                        // For standard container std::vector

/**
 * @brief Structure to hold shared initialization data for the system.
 *
 * This structure aggregates pointers to essential system modules that are initialized
 * during the startup phase. It includes:
 *   - EEPROMStorage for persistent storage of critical parameters.
 *   - Controller, the central decision-making module that processes sensor data
 *     and commands actuators.
 *   - UI, the module responsible for user interactions and display.
 *   - A pointer to a vector of sensor objects that implement the ISensor interface.
 *
 * This structure is populated during system initialization (setupTask) and then
 * passed to other components that require access to these shared objects.
 */
struct InitDataStruct {
    std::shared_ptr<EEPROMStorage> eepromStore;           ///< Pointer to the EEPROM storage module for persistence.
    std::shared_ptr<Controller> controller;               ///< Pointer to the Controller module responsible for control logic.
    std::shared_ptr<UI> ui;                               ///< Pointer to the UI module handling local user interface.
    std::vector<std::shared_ptr<ISensor>>* sensorList;    ///< Pointer to a vector containing all sensor modules implementing ISensor.
};

#endif // INIT_DATA_H