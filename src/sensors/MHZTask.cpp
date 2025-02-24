#include "MHZTask.h"
#include "FreeRTOS.h"
#include "task.h"
#include <cstdio> // for sprintf or debugging if needed

#include "sensors/MHZ19CSensor.h"
#include "uart/PicoOsUart.hpp"
#include "MHZTaskParams.h"

extern "C" void MHZTask(void *pvParameters)
{
    // 1) Cast the parameter
    auto params = static_cast<MHZTaskParams*>(pvParameters);

    // 2) Create the sensor using the sensorUart
    MHZ19CSensor co2Sensor(params->sensorUart);

    // Optional: Log that we're starting
    params->logUart->send("[MHZTask] Started MHZ19C sensor task...\r\n");

    // 3) Initialize the sensor (warmup, calibration, etc.)
    co2Sensor.initSensor();
    co2Sensor.setAutoCalibration(false);

    // 4) Main loop: read sensor, log results
    while (true) {
        bool success = co2Sensor.readSensor();
        if (success) {
            int co2ppm = co2Sensor.getCO2();

            // Format a message
            char msg[64];
            snprintf(msg, sizeof(msg), "[MHZTask] CO2: %d ppm\r\n", co2ppm);
            // Send via logUart (same logic as i2cTask)
            params->logUart->send(msg);

        } else {
            params->logUart->send("[MHZTask] CO2 read failed or invalid\r\n");
        }

        // Delay 2s
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
