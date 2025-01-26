//
// Created by natal on 26.1.2025.
//

#ifndef GREENHOUSE_CONTROLLER_H
#define GREENHOUSE_CONTROLLER_H

#include <cstdint>
#include <memory>

class CO2Sensor;
class TempRHSensor;
class PressureSensor;
class FanDriver;
class ValveDriver;

/**
 * @brief High-level greenhouse control logic.
 *
 * Periodically:
 *  - Read sensor data
 *  - Compare CO2 reading to set-point
 *  - Adjust fan speed if CO2 > 2000 ppm (safety)
 *  - Open/close valve in short bursts if below set-point, etc.
 */
class Controller {
public:
    Controller(std::shared_ptr<CO2Sensor> co2,
               std::shared_ptr<TempRHSensor> thr,
               std::shared_ptr<PressureSensor> pres,
               std::shared_ptr<FanDriver> fan,
               std::shared_ptr<ValveDriver> valve);

    /**
     * @brief Called periodically (e.g. once per second) by a FreeRTOS task.
     */
    void updateControl();

    void setCO2Setpoint(float co2ppm) { co2Setpoint_ = co2ppm; }
    float getCO2Setpoint() const      { return co2Setpoint_; }

    // Accessors for current measured or commanded values
    float getCurrentCO2() const       { return currentCO2_; }
    float getCurrentTemp() const      { return currentTemp_; }
    float getCurrentRH() const        { return currentRH_; }
    float getPressureDiff() const     { return currentPressure_; }
    float getCurrentFanSpeed() const  { return currentFanSpeed_; }
    bool  isValveOpen() const;

private:
    // Sensors
    std::shared_ptr<CO2Sensor>       co2Sensor_;
    std::shared_ptr<TempRHSensor>    thrSensor_;
    std::shared_ptr<PressureSensor>  presSensor_;

    // Actuators
    std::shared_ptr<FanDriver>   fan_;
    std::shared_ptr<ValveDriver> valve_;

    // Control variables
    float co2Setpoint_;
    float currentCO2_;
    float currentTemp_;
    float currentRH_;
    float currentPressure_;
    float currentFanSpeed_;
};

#endif //GREENHOUSE_CONTROLLER_H
