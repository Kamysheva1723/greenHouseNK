#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <memory>
#include "FreeRTOS.h"
#include "timers.h"

/*
   Controller Module Header

   This module encapsulates the central logic of the Greenhouse Fertilization System.
   It aggregates sensor data from various sensors (CO₂, temperature & humidity, pressure)
   and uses this information to control actuators (fan and CO₂ valve) to maintain the optimal 
   greenhouse environment. It includes safety overrides (e.g., for high CO₂ levels), and uses 
   a FreeRTOS one-shot timer to automatically close the valve after a predetermined time.
*/

// Forward declarations of sensor and driver classes for interfacing with hardware.
class CO2Sensor;
class TempRHSensor;
class PressureSensor;
class FanDriver;
class ValveDriver;
class EEPROMStorage;

class Controller {
public:
    /*
       Constructor:
       Initializes the Controller with shared pointers to the sensor modules (CO₂, TempRH, Pressure)
       and the actuator drivers (Fan, Valve) as well as EEPROM storage for persistence. It also loads 
       the initial CO₂ setpoint from EEPROM if available, otherwise uses a default value. Additionally,
       it creates a one-shot FreeRTOS timer (valveTimer) to automatically close the valve after 2 seconds 
       of being open.
    */
    Controller(std::shared_ptr<CO2Sensor> co2,
               std::shared_ptr<TempRHSensor> thr,
               std::shared_ptr<PressureSensor> pres,
               std::shared_ptr<FanDriver> fan,
               std::shared_ptr<ValveDriver> valve,
               std::shared_ptr<EEPROMStorage> eeprom);

    /*
       updateControl():
       Main control loop method that should be called periodically (e.g., by sensorTask).
       It performs the following:
         1. Reads the latest sensor values.
         2. Checks for safety conditions (e.g., CO₂ > 2000 ppm) and acts accordingly.
         3. Controls the fan and CO₂ valve based on whether the current CO₂ level is below or above the setpoint.
         4. Uses a cooldown period (30 seconds) to prevent rapid valve cycling.
         5. Initiates a one-shot timer to automatically close the valve after 2 seconds.
         6. Prints debug information for monitoring purposes.
    */
    void updateControl();

    /*
       setCO2Setpoint():
       Updates the target CO₂ setpoint and persists it in EEPROM so that the system remembers the setting
       across power cycles.
    */
    void setCO2Setpoint(float setpoint);

    // Getter methods to retrieve the current CO₂ setpoint and most recent sensor and actuator readings.
    float getCO2Setpoint() const;
    float getCurrentCO2() const;
    float getCurrentTemp() const;
    float getCurrentRH() const;
    float getCurrentPressure() const;
    float getCurrentFanSpeed() const;
    bool  isValveOpen() const;

private:
    // --- Shared pointers to sensor and driver objects ---
    std::shared_ptr<CO2Sensor>       co2Sensor;      // CO₂ sensor using Modbus RTU protocol.
    std::shared_ptr<TempRHSensor>    thrSensor;      // Temperature & Humidity sensor.
    std::shared_ptr<PressureSensor>  presSensor;     // Differential pressure sensor via I²C.
    std::shared_ptr<FanDriver>       fan;            // Fan driver that controls fan speed via Modbus.
    std::shared_ptr<ValveDriver>     valve;          // Valve driver for CO₂ valve control via GPIO.
    std::shared_ptr<EEPROMStorage>   eepromStorage;  // EEPROM for persistent storage of parameters.

    // --- Control setpoints and state variables ---
    float co2Setpoint;         // Target CO₂ concentration in ppm.
    float currentCO2       = 0.0f; // Latest CO₂ reading.
    float currentTemp      = 0.0f; // Latest temperature reading (in °C).
    float currentRH        = 0.0f; // Latest relative humidity reading (%).
    float currentPressure  = 0.0f; // Latest pressure reading (in Pascals).
    float currentFanSpeed  = 0.0f; // Last commanded fan speed (%).

    bool  safetyVent       = false; // Flag to indicate if safety override is active.

    // --- FreeRTOS timer for auto-closing the CO₂ valve ---
    TimerHandle_t valveTimer;  // One-shot timer that closes the valve after 2s.
    
    /*
       valveTimerCallback():
       Static callback function for the valveTimer. When fired, it retrieves the associated Controller 
       object (via the timer ID) and, if the valve is still open, closes it and updates the last closure tick.
    */
    static void valveTimerCallback(TimerHandle_t xTimer);

    // --- Cooldown management ---
    /*
       lastValveCloseTick:
       Records the tick count at which the valve was last closed. This is used 
       to enforce a cooldown period (e.g., 30 seconds) before the valve can be reopened.
    */
    uint32_t lastValveCloseTick = 0;

    // The cooldown period (in milliseconds) that must elapse after closing the valve before it can be opened again.
    static constexpr uint32_t VALVE_COOLDOWN_MS = 30000;
};

#endif // CONTROLLER_H
