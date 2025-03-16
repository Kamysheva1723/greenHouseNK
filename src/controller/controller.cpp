#include "Controller.h"
#include "./sensors/CO2Sensor.h"
#include "./sensors/TempRHSensor.h"
#include "./sensors/PressureSensor.h"
#include "./FanDriver/FanDriver.h"
#include "./ValveDriver/ValveDriver.h"
#include "EEPROM/EEPROMStorage.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <cstdio>
#include "hardware/gpio.h"

/*
   The Controller module is the central decision‐maker of the Greenhouse Fertilization System.
   It gathers sensor data, executes the control logic to compare sensor values with the target CO₂ setpoint,
   and then commands actuators (the fan and CO₂ valve) accordingly.
   It also integrates safety overrides (e.g., if CO₂ exceeds 2000 ppm) and uses a one-shot FreeRTOS timer
   to automatically close the CO₂ valve after it has been open for a defined period.
*/

Controller::Controller(std::shared_ptr<CO2Sensor> co2,
                       std::shared_ptr<TempRHSensor> thr,
                       std::shared_ptr<PressureSensor> pres,
                       std::shared_ptr<FanDriver> fan,
                       std::shared_ptr<ValveDriver> valve,
                       std::shared_ptr<EEPROMStorage> eeprom)
        : co2Sensor(co2)
        , thrSensor(thr)
        , presSensor(pres)
        , fan(fan)
        , valve(valve)
        , eepromStorage(eeprom)
{
    // Initialize the CO₂ setpoint from EEPROM. If EEPROM storage is not available,
    // then a default CO₂ setpoint of 1500 ppm is used.
    if (eepromStorage) {
        co2Setpoint = static_cast<float>(eepromStorage->loadCO2Setpoint());
        printf("[Controller] Initial CO₂ setpoint from EEPROM: %.1f\n", co2Setpoint);
    } else {
        co2Setpoint = 1500.0f; // Fallback default setpoint in ppm
        printf("[Controller] EEPROM not available, using default CO₂ setpoint: %.1f\n", co2Setpoint);
    }

    safetyVent = false; // Safety override flag, initially inactive

    // --- Create a one-shot software timer (FreeRTOS timer) to automatically close the CO₂ valve after 2 seconds ---
    // A one-shot timer is created so that once opened, the valve will only be left open for a maximum of 2 seconds,
    // preventing over-fertilization.
    valveTimer = xTimerCreate(
            "ValveTimer",                      // Timer name (for debugging)
            pdMS_TO_TICKS(2000),               // Timer period: 2000 ms (2 seconds)
            pdFALSE,                           // One-shot timer (does not auto-reload)
            this,                              // Timer ID is set to this Controller instance (allows callback to access instance)
            &Controller::valveTimerCallback    // Callback function to be invoked when the timer expires
    );

    if (valveTimer == nullptr) {
        printf("[Controller] Error creating valveTimer!\n");
    }

    // Initialize lastValveCloseTick with the current tick count so that the valve can be opened immediately on startup if required.
    lastValveCloseTick = xTaskGetTickCount();
}

/*
   Valve Timer Callback Function:
   This is invoked when the one-shot timer expires (after 2 seconds).
   It checks if the valve is still open, and if so, closes it and updates the last closure tick.
*/
void Controller::valveTimerCallback(TimerHandle_t xTimer) {
    // Retrieve the Controller instance from the timer ID
    Controller* ctrl = static_cast<Controller*>(pvTimerGetTimerID(xTimer));
    if (ctrl && ctrl->valve && ctrl->valve->isOpen()) {
        ctrl->valve->closeValve(); // Close the valve
        // Record the time when the valve was closed (for cooldown purposes)
        ctrl->lastValveCloseTick = xTaskGetTickCount();
        printf("[Controller] Valve closed after 2s open period.\n");
    }
}

/*
   updateControl():
   This function contains the main control loop logic called periodically (by sensorTask).
   It reads current sensor values, applies safety checks and standard control logic, and then
   commands the actuators (fan and valve) accordingly.
*/
void Controller::updateControl() {
    // 1) Read sensor values from the sensor modules if available.
    if (co2Sensor) {
        currentCO2 = co2Sensor->getCO2ppm();
    }
    if (thrSensor) {
        currentTemp = thrSensor->getTemperature();
        currentRH   = thrSensor->getHumidity();
    }
    if (presSensor) {
        currentPressure = presSensor->getPressurePa();
    }

    // 2) Safety Override:
    //    If the current CO₂ concentration exceeds 2000 ppm, engage a safety override.
    if (currentCO2 > 2000.0f) {
        // Force the fan to operate at 100% speed to reduce CO₂ levels quickly.
        if (fan) {
            fan->setFanSpeed(100.0f);
            currentFanSpeed = 100.0f;
            safetyVent = true; // Activate safety override state
        }
        // If the CO₂ valve is currently open, close it immediately and record the closure tick.
        if (valve && valve->isOpen()) {
            valve->closeValve();
            lastValveCloseTick = xTaskGetTickCount();
        }
        printf("[Controller] *** CO₂ > 2000: Forcing valve closed and fan at 100%%\n");
        return; // Exit control update early since safety override is active.
    }
    // If not in safety mode and safetyVent flag is not active, ensure fan remains off.
    else if (!safetyVent) {
        if (fan) {
            fan->setFanSpeed(0.0f);
            currentFanSpeed = 0.0f;
        }
        printf("[Controller] *** CO₂ < 2000: fan at 0%%\n");
    }

    // If safety override was active and CO₂ falls back to or below the setpoint,
    // then clear the safety override and turn fan off.
    if (safetyVent && currentCO2 <= co2Setpoint) {
        if (fan) {
            fan->setFanSpeed(0.0f);
            currentFanSpeed = 0.0f;
        }
        safetyVent = false; // Clear safety override
    }

    // 3) Valve Control Logic:
    // Determine if additional CO₂ is needed based on the current CO₂ level compared to the setpoint.
    bool needCO2 = (currentCO2 < co2Setpoint);

    if (needCO2) {
        // Before opening the valve, check if the 30-second cooldown period has elapsed since it was last closed.
        uint32_t now = xTaskGetTickCount();
        uint32_t elapsed = now - lastValveCloseTick; // Calculate elapsed time

        if (valve && !valve->isOpen()) {
            // Only open the valve if the cooldown period has been satisfied.
            if (elapsed >= pdMS_TO_TICKS(VALVE_COOLDOWN_MS)) {
                valve->openValve();
                printf("[Controller] Opening valve (CO₂=%.1f < set=%.1f)\n", currentCO2, co2Setpoint);

                // Restart the one-shot timer to ensure the valve is closed automatically after 2 seconds.
                xTimerStop(valveTimer, 0);
                xTimerStart(valveTimer, 0);
            } else {
                // If the cooldown period has not finished, print remaining cooldown time.
                printf("[Controller] Valve cooldown active: %lu ms left.\n",
                       (pdMS_TO_TICKS(VALVE_COOLDOWN_MS) - elapsed));
            }
        }
        // If the valve is already open, let the timer handle the automatic closing.
    } else {
        // When current CO₂ is at or exceeds the setpoint, ensure the valve is closed.
        if (valve && valve->isOpen()) {
            valve->closeValve();
            // Update lastValveCloseTick to record the time of closing (for subsequent cooldown period).
            lastValveCloseTick = xTaskGetTickCount();
            printf("[Controller] Valve closed early (CO₂=%.1f >= set=%.1f)\n", currentCO2, co2Setpoint);
            // Stop the valve timer since the valve has been closed.
            xTimerStop(valveTimer, 0);
        }
    }

    // 4) Debug print: Print current sensor readings and actuator states for monitoring.
    printf("[Controller] CO₂=%.1f, set=%.1f, valve=%s, fan=%.1f, temp=%.1f, RH=%.1f\n",
           currentCO2,
           co2Setpoint,
           (valve && valve->isOpen()) ? "OPEN" : "CLOSED",
           currentFanSpeed,
           currentTemp,
           currentRH);
}

/*
   setCO2Setpoint():
   This method updates the target CO₂ setpoint for the system both in the Controller instance 
   and persistently in the EEPROM storage (if available) so that settings are retained across reboots.
*/
void Controller::setCO2Setpoint(float setpoint) {
    co2Setpoint = setpoint;
    if (eepromStorage) {
        // Persist the new setpoint to EEPROM as a 16-bit value.
        eepromStorage->storeCO2Setpoint(static_cast<uint16_t>(setpoint));
    }
}

/*
   getCO2Setpoint(), getCurrentCO2(), getCurrentTemp(), getCurrentRH(), getCurrentPressure(), getCurrentFanSpeed():
   These methods return the current target setpoint and most recent sensor readings or actuator statuses.
*/
float Controller::getCO2Setpoint() const {
    return co2Setpoint;
}

float Controller::getCurrentCO2() const {
    return currentCO2;
}

float Controller::getCurrentTemp() const {
    return currentTemp;
}

float Controller::getCurrentRH() const {
    return currentRH;
}

float Controller::getCurrentPressure() const {
    return currentPressure;
}

float Controller::getCurrentFanSpeed() const {
    return currentFanSpeed;
}

/*
   isValveOpen():
   Returns true if the CO₂ valve is currently open, false otherwise.
*/
bool Controller::isValveOpen() const {
    return valve && valve->isOpen();
}