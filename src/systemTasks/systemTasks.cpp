#include "systemTasks.h"
#include "FreeRTOS.h"
#include "task.h"
#include <cstdio>
#include <vector>

#include "sensors/ISensor.h"
#include "controller/Controller.h"
#include "UI/ui.h"
#include "cloud/cloud.h"
#include "FanDriver/FanDriver.h"
#include "ValveDriver/ValveDriver.h"
#include "EEPROM/EEPROMStorage.h"
#include "init-data.h"
#include "rot/GpioEvent.h"
#include "queue.h"

extern QueueHandle_t xGpioQueue; // declared globally

// -----------------------------------------------------------------------------
// sensorTask
// -----------------------------------------------------------------------------
void sensorTask(void *param) {
    printf("sensorTask started in task: %s\n", pcTaskGetName(nullptr));

    auto initData = static_cast<InitDataStruct*>(param);
    auto sensorList = initData->sensorList;
    auto ctrl       = initData->controller;

    printf("_______SENSOR TASK______\n");

    while (true) {
        // 1) Read all sensors
        if (sensorList) {
            for (auto &sensor: *sensorList) {
                if (sensor) {
                    sensor->readSensor();
                }
            }
        }

        // 2) Update control logic
        if (ctrl) {
            ctrl->updateControl();
        }

        // Sleep some time
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// -----------------------------------------------------------------------------
// uiTask
// -----------------------------------------------------------------------------
void uiTask(void *param) {
    UI* ui = static_cast<UI*>(param);
    const TickType_t refreshDelay = pdMS_TO_TICKS(1000);

    while (true) {
        if (ui) {
            ui->updateUI();
        }
        vTaskDelay(refreshDelay);
    }
}

// -----------------------------------------------------------------------------
// eepromTask
// -----------------------------------------------------------------------------
void eepromTask(void *param) {
    printf("eepromTask started in task: %s\n", pcTaskGetName(nullptr));
    auto eeprom = static_cast<EEPROMStorage*>(param);
    if (!eeprom) {
        printf("eepromTask: invalid EEPROM pointer\n");
        vTaskDelete(nullptr);
        return;
    }

    // Optionally do some early checks
    vTaskDelay(pdMS_TO_TICKS(5000));
    while (true) {
        // Periodic or background EEPROM logic here
        vTaskDelay(pdMS_TO_TICKS(50000));
    }
}

// -----------------------------------------------------------------------------
// initTask
// -----------------------------------------------------------------------------
void initTask(void *param) {
    printf("initTask started in task: %s\n", pcTaskGetName(nullptr));
    auto initData   = static_cast<InitDataStruct*>(param);
    auto eeprom     = initData->eepromStore;
    auto controller = initData->controller;
    auto ui         = initData->ui;

    printf("initTask: Starting initialization...\n");
    uint16_t sp = eeprom->loadCO2Setpoint();
    printf("initTask: read CO2 setpoint from EEPROM = %u\n", sp);

    if (controller) {
        controller->setCO2Setpoint(static_cast<float>(sp));
    }
    if (ui) {
        ui->setLocalSetpoint(static_cast<float>(sp));
    }
    printf("initTask: Initialization complete.\n");

    vTaskDelete(nullptr);
}

// -----------------------------------------------------------------------------
// rotaryEventTask
// -----------------------------------------------------------------------------
void rotaryEventTask(void *param) {
    UI* ui = static_cast<UI*>(param);
    GpioEvent evt;

    while (true) {
        // Wait indefinitely for an event
        if (xQueueReceive(xGpioQueue, &evt, portMAX_DELAY) == pdTRUE) {
            switch (evt.type) {
                case EventType::TURN: {
                    int delta = (evt.clockwise ? 1 : -1);
                    ui->onEncoderTurn(delta);
                    break;
                }
                case EventType::PRESS: {
                    ui->onButtonPress();
                    break;
                }
                default:
                    // ignore unknown
                    break;
            }
        }
    }
}

// -----------------------------------------------------------------------------
// cloudTask
// -----------------------------------------------------------------------------
void cloudTask(void* param) {
    printf("cloudTask started in task: %s\n", pcTaskGetName(nullptr));

    // The 'param' should be a pointer to your Cloud object
    Cloud* cloud = static_cast<Cloud*>(param);
    if (!cloud) {
        printf("[cloudTask] ERROR: No valid Cloud pointer!\n");
        vTaskDelete(nullptr);
        return;
    }


    while (true) {
        if (cloud->updateSensorData()) {
            printf("[cloudTask] Sensor data updated successfully.\n");
        } else {
            printf("[cloudTask] Failed to update sensor data.\n");
        }


        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}
