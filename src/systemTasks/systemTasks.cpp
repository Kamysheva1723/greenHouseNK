#include "systemTasks.h"            // Include header for system tasks declarations
#include "FreeRTOS.h"               // FreeRTOS kernel API
#include "task.h"                   // FreeRTOS task related functions
#include <cstdio>                   // Standard C library for printf, etc.
#include <vector>                   // STL vector container

#include "sensors/ISensor.h"        // Interface for sensor modules
#include "controller/Controller.h"  // Controller module header
#include "UI/ui.h"                  // User Interface module header
#include "cloud/cloud.h"            // Cloud connectivity module header
#include "FanDriver/FanDriver.h"      // Fan driver module header
#include "ValveDriver/ValveDriver.h"  // Valve driver module header
#include "EEPROM/EEPROMStorage.h"   // EEPROM storage module header
#include "init-data.h"              // Shared initialization data structure header
#include "rot/GpioEvent.h"          // GPIO event definitions for rotary encoder events
#include "queue.h"                  // FreeRTOS queue API

extern QueueHandle_t xGpioQueue;     // Externally declared global queue handle for GPIO events

// -----------------------------------------------------------------------------
// sensorTask
// -----------------------------------------------------------------------------
//
// This task interfaces with all sensor modules (e.g., CO₂, Temperature, Humidity, Pressure).
// It invokes each sensor's readSensor() method and thereafter calls the controller's updateControl()
// to update the system control logic based on fresh sensor readings.
// The sensor read cycle operates periodically with a 500 ms delay.
void sensorTask(void *param) {
    // Log task start and current task name for debugging purposes.
    printf("sensorTask started in task: %s\n", pcTaskGetName(nullptr));

    // Cast the parameter to the initialization data structure provided at startup.
    auto initData = static_cast<InitDataStruct*>(param);
    // Extract the pointer to the sensor list and the controller instance.
    auto sensorList = initData->sensorList;
    auto ctrl       = initData->controller;

    printf("_______SENSOR TASK______\n");

    // Infinite loop: periodically read sensor data and update control logic.
    while (true) {
        // 1) Iterate through each sensor in the sensor list and invoke its readSensor() method.
        if (sensorList) {
            for (auto &sensor: *sensorList) {
                if (sensor) {
                    sensor->readSensor();
                }
            }
        }

        // 2) Invoke controller's updateControl() method to process new sensor readings.
        if (ctrl) {
            ctrl->updateControl();
        }

        // 3) Delay for 500 ms (converted to ticks) before the next sensor reading cycle.
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// -----------------------------------------------------------------------------
// uiTask
// -----------------------------------------------------------------------------
//
// This task is responsible for updating the user interface periodically.
// It calls the updateUI() function of the UI module every 1000 ms.
// The updateUI() function is responsible for refreshing the OLED display based on current sensor
// readings and system state.
void uiTask(void *param) {
    // Cast the parameter to a pointer to the UI module.
    UI* ui = static_cast<UI*>(param);
    // Define display refresh delay time: 1000 milliseconds.
    const TickType_t refreshDelay = pdMS_TO_TICKS(1000);

    // Task loop: continuously update the display.
    while (true) {
        if (ui) {
            ui->updateUI();
        }
        // Delay for 1000 ms between display updates to ensure timely refreshes.
        vTaskDelay(refreshDelay);
    }
}

// -----------------------------------------------------------------------------
// eepromTask
// -----------------------------------------------------------------------------
//
// This task handles background operations relating to EEPROM storage, such as periodic maintenance.
// It starts after an initial delay, then loops with a long delay between cycles to minimize resource usage.
void eepromTask(void *param) {
    // Print task start message with task name for debugging.
    printf("eepromTask started in task: %s\n", pcTaskGetName(nullptr));
    // Cast parameter to EEPROMStorage pointer.
    auto eeprom = static_cast<EEPROMStorage*>(param);
    if (!eeprom) {
        printf("eepromTask: invalid EEPROM pointer\n");
        // Terminate task if EEPROM pointer is invalid.
        vTaskDelete(nullptr);
        return;
    }

    // Initial delay of 5 seconds before starting background EEPROM operations.
    vTaskDelay(pdMS_TO_TICKS(5000));
    while (true) {
        // Insert periodic EEPROM operations here, if needed (e.g., data validation, periodic writes).
        vTaskDelay(pdMS_TO_TICKS(50000)); // Delay of 50 seconds between cycles.
    }
}

// -----------------------------------------------------------------------------
// initTask
// -----------------------------------------------------------------------------
//
// The initTask is responsible for system initialization routines.
// It reads the stored CO₂ setpoint from EEPROM and synchronizes this value with both 
// the Controller (for control logic) and the UI (for display).
// After completing initialization, the task deletes itself.
void initTask(void *param) {
    printf("initTask started in task: %s\n", pcTaskGetName(nullptr));
    // Cast parameter to the shared InitDataStruct structure.
    auto initData   = static_cast<InitDataStruct*>(param);
    auto eeprom     = initData->eepromStore;
    auto controller = initData->controller;
    auto ui         = initData->ui;

    printf("initTask: Starting initialization...\n");
    // Load the CO₂ setpoint from the EEPROM storage.
    uint16_t sp = eeprom->loadCO2Setpoint();
    printf("initTask: read CO2 setpoint from EEPROM = %u\n", sp);

    // Update the Controller's CO₂ setpoint.
    if (controller) {
        controller->setCO2Setpoint(static_cast<float>(sp));
    }
    // Update the UI's local setpoint copy.
    if (ui) {
        ui->setLocalSetpoint(static_cast<float>(sp));
    }
    printf("initTask: Initialization complete.\n");

    // Delete this initialization task as its work is complete.
    vTaskDelete(nullptr);
}

// -----------------------------------------------------------------------------
// rotaryEventTask
// -----------------------------------------------------------------------------
//
// This task processes rotary encoder events captured from hardware interrupts.
// It uses a FreeRTOS queue (xGpioQueue) to receive events asynchronously.
// Depending on the event type, it calls the appropriate function on the UI module (e.g., onEncoderTurn, onButtonPress).
void rotaryEventTask(void *param) {
    // Cast parameter to UI pointer.
    UI* ui = static_cast<UI*>(param);
    // Create an instance to store received GPIO events.
    GpioEvent evt;

    while (true) {
        // Block indefinitely until an event is received from the queue.
        if (xQueueReceive(xGpioQueue, &evt, portMAX_DELAY) == pdTRUE) {
            // Process event based on its type.
            switch (evt.type) {
                case EventType::TURN: {
                    // Determine rotation direction: +1 for clockwise, -1 for counter-clockwise.
                    int delta = (evt.clockwise ? 1 : -1);
                    ui->onEncoderTurn(delta);
                    break;
                }
                case EventType::PRESS: {
                    // Process button press event.
                    ui->onButtonPress();
                    break;
                }
                default:
                    // Ignore any unknown event types.
                    break;
            }
        }
    }
}

// -----------------------------------------------------------------------------
// cloudTask
// -----------------------------------------------------------------------------
//
// This task manages cloud connectivity and sends sensor data to a remote server.
// It calls the updateSensorData() method of the Cloud module periodically (every 60 seconds).
// The Cloud module uses secure TLS connections for data transmission and remote command handling.
void cloudTask(void* param) {
    printf("cloudTask started in task: %s\n", pcTaskGetName(nullptr));

    // Cast parameter to Cloud pointer.
    Cloud* cloud = static_cast<Cloud*>(param);
    if (!cloud) {
        printf("[cloudTask] ERROR: No valid Cloud pointer!\n");
        vTaskDelete(nullptr);
        return;
    }

    // Main loop: periodically update sensor data to the cloud.
    while (true) {
        // Call the cloud update function to transmit sensor data.
        if (cloud->updateSensorData()) {
            printf("[cloudTask] Sensor data updated successfully.\n");
        } else {
            printf("[cloudTask] Failed to update sensor data.\n");
        }

        // Wait for 60 seconds before the next cloud update.
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}