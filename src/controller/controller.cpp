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

}


void Controller::updateControl() {
    // No need to re-read the setpoint every update. Use the stored value.

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

    // 2) Safety override: if CO₂ > 2000, force fan to 100% and close the valve.
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
    }else if (!safetyVent){
        fan->setFanSpeed(0.0f);
        currentFanSpeed = 0.0f;
        printf("[Controller] *** CO₂ < 2000: fan at 0%%\n");
    }

    if (safetyVent && currentCO2 <= co2Setpoint) {
        fan->setFanSpeed(0.0f);
        currentFanSpeed = 0.0f;
        safetyVent = false;
    }



    // 3) Valve control logic.
    static TickType_t valveOpenStartTick = 0;
    bool needCO2 = (currentCO2 < co2Setpoint);

    if (needCO2) {
        if (valve && !valve->isOpen()) {
            valve->openValve();
            valveOpenStartTick = xTaskGetTickCount();
            printf("[Controller] Opening valve (CO₂=%.1f < set=%.1f)\n", currentCO2, co2Setpoint);
        } else if (valve && valve->isOpen()) {
            TickType_t elapsed = xTaskGetTickCount() - valveOpenStartTick;
            if (elapsed >= pdMS_TO_TICKS(2000)) {
                valve->closeValve();
                printf("[Controller] Valve closed after 2s open period.\n");
            }
        }
    } else {
        if (valve && valve->isOpen()) {
            valve->closeValve();
            printf("[Controller] Valve closed early (CO₂=%.1f >= set=%.1f)\n", currentCO2, co2Setpoint);
        }
    }

    // 4) Fan control logic.
    //if (fan) {
    //    float diff = currentCO2 - co2Setpoint;
    //    float speed = 0.0f;
    //    if (diff > 0) {
    //        speed = (diff > 300) ? 60.0f : 30.0f;
    //    }
    //    fan->setFanSpeed(speed);
    //    currentFanSpeed = speed;
    //}

    // 5) Debug print.
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

