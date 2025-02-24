#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <memory>

class CO2Sensor;
class TempRHSensor;
class PressureSensor;
class FanDriver;
class ValveDriver;
class EEPROMStorage; // forward declaration

class Controller {
public:
    Controller(std::shared_ptr<CO2Sensor> co2,
               std::shared_ptr<TempRHSensor> thr,
               std::shared_ptr<PressureSensor> pres,
               std::shared_ptr<FanDriver> fan,
               std::shared_ptr<ValveDriver> valve,
               std::shared_ptr<EEPROMStorage> eeprom);
    void updateControl();
    void setCO2Setpoint(float setpoint);
    float getCO2Setpoint() const;
    float getCurrentCO2() const;
    float getCurrentTemp() const;
    float getCurrentRH() const;
    float getCurrentPressure() const;
    float getCurrentFanSpeed() const;
    bool isValveOpen() const;
    bool safetyVent;


private:
    std::shared_ptr<CO2Sensor> co2Sensor;
    std::shared_ptr<TempRHSensor> thrSensor;
    std::shared_ptr<PressureSensor> presSensor;
    std::shared_ptr<FanDriver> fan;
    std::shared_ptr<ValveDriver> valve;
    std::shared_ptr<EEPROMStorage> eepromStorage; // new member

    float co2Setpoint;
    float currentCO2;
    float currentTemp;
    float currentRH;
    float currentPressure;
    float currentFanSpeed;



};

#endif // CONTROLLER_H

