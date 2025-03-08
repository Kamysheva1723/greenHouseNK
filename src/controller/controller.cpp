#include "Controller.h"
#include "./sensors/CO2Sensor.h"
#include "./sensors/TempRHSensor.h"
#include "./sensors/PressureSensor.h"
#include "./FanDriver/FanDriver.h"
#include "./ValveDriver/ValveDriver.h"
#include "EEPROM/EEPROMStorage.h"  // include EEPROM header
#include "FreeRTOS.h"
#include <cstdio>
#include "task.h"
#include "hardware/gpio.h"
#include "timers.h"

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
    if (eepromStorage) {
        co2Setpoint = static_cast<float>(eepromStorage->loadCO2Setpoint());
        printf("[Controller] Initial CO₂ setpoint loaded from EEPROM: %.1f\n", co2Setpoint);
    } else {
        co2Setpoint = 1500.0f; // Fallback default if EEPROM is not available
        printf("[Controller] EEPROM not available, using default CO₂ setpoint: %.1f\n", co2Setpoint);
    }
    safetyVent = false;
    valveTimer = nullptr;
    // Create a one-shot timer for the valve with a period of 2000ms.
    // The timer ID is set to 'this' so that we can access the Controller object in the callback.
    valveTimer = xTimerCreate(
            "ValveTimer",                      // Timer name for debugging.
            pdMS_TO_TICKS(2000),               // Timer period: 2000 ms (2 seconds).
            pdFALSE,                           // Auto-reload is disabled (one-shot timer).
            this,                              // Timer ID: pointer to this Controller instance.
            &Controller::valveTimerCallback    // Timer callback function.
    );

    if (valveTimer == nullptr) {
        // Handle timer creation error if needed.
        printf("[Controller] Error creating valveTimer!\n");
    }

}

void Controller::valveTimerCallback(TimerHandle_t xTimer) {
    // Retrieve the pointer to the Controller object from the timer ID.
    Controller* ctrl = static_cast<Controller*>(pvTimerGetTimerID(xTimer));
    if (ctrl && ctrl->valve && ctrl->valve->isOpen()) {
        ctrl->valve->closeValve();
        printf("[Controller] Valve closed after 2s open period.\n");
    }
}


void Controller::updateControl() {
    // 1) Read sensor values.
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

    // 2) Safety override:
    // If CO₂ > 2000, force fan to 100% and close the valve.
    if (currentCO2 > 2000.0f) {
        if (fan) {
            fan->setFanSpeed(100.0f);
            currentFanSpeed = 100.0f;
            safetyVent = true;
        }
        if (valve && valve->isOpen()) {
            valve->closeValve();
        }
        printf("[Controller] *** CO₂ > 2000: Forcing valve closed and fan at 100%%\n");
        return;
    } else if (!safetyVent) {
        // If not in safety mode, set fan speed to 0%.
        if (fan) {
            fan->setFanSpeed(0.0f);
            currentFanSpeed = 0.0f;
        }
        printf("[Controller] *** CO₂ < 2000: fan at 0%%\n");
    }

    // If safetyVent is active and current CO₂ is below the setpoint,
    // turn off the fan and clear safetyVent.
    if (safetyVent && currentCO2 <= co2Setpoint) {
        if (fan) {
            fan->setFanSpeed(0.0f);
            currentFanSpeed = 0.0f;
        }
        safetyVent = false;
    }

    // 3) Valve control logic using the FreeRTOS timer.
    bool needCO2 = (currentCO2 < co2Setpoint);

    if (needCO2) {
        // If CO₂ is below the setpoint, we need to open the valve.
        if (valve && !valve->isOpen()) {
            valve->openValve();
            printf("[Controller] Opening valve (CO₂=%.1f < set=%.1f)\n", currentCO2, co2Setpoint);

            // Stop any running timer and start a new one to automatically close the valve after 2 seconds.
            xTimerStop(valveTimer, 0);
            xTimerStart(valveTimer, 0);
        }
        // If the valve is already open, do nothing.
        // The timer will automatically close it after 2 seconds.
    } else {
        // If CO₂ is at or above the setpoint, close the valve immediately.
        if (valve && valve->isOpen()) {
            valve->closeValve();
            printf("[Controller] Valve closed early (CO₂=%.1f >= set=%.1f)\n", currentCO2, co2Setpoint);

            // Stop the timer as it is no longer needed.
            xTimerStop(valveTimer, 0);
        }
    }

    // 4) Debug print: output the current sensor values and control states.
    printf("[Controller] CO₂=%.1f, set=%.1f, valve=%s, fan=%.1f, temp=%.1f, RH=%.1f\n",
           currentCO2,
           co2Setpoint,
           (valve && valve->isOpen()) ? "OPEN" : "CLOSED",
           currentFanSpeed,
           currentTemp,
           currentRH);
}



void Controller::setCO2Setpoint(float setpoint) {
    co2Setpoint = setpoint;
    // Store the new setpoint in EEPROM.
    if (eepromStorage) {
        eepromStorage->storeCO2Setpoint(static_cast<uint16_t>(setpoint));
    }
}

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

bool Controller::isValveOpen() const {
    return valve && valve->isOpen();
}

