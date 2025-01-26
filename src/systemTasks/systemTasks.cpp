//
// Created by natal on 26.1.2025.
//

#include "SystemTasks.h"
#include "Controller.h"
#include "UI.h"
#include "CloudClient.h"
#include "Sensors.h"
#include "FanDriver.h"
#include <cstdio>
#include "FreeRTOS.h"
#include "task.h"

void sensorTask(void *param) {
    // param might hold references to sensor objects
    auto sensors = reinterpret_cast<std::vector<ISensor*>*>(param);

    const TickType_t delay = pdMS_TO_TICKS(1000);
    for(;;) {
        for(auto &s : *sensors) {
            s->readSensor();
        }
        vTaskDelay(delay);
    }
}

void controlTask(void *param) {
    auto ctrl = reinterpret_cast<Controller*>(param);
    const TickType_t delay = pdMS_TO_TICKS(1000);
    for (;;) {
        ctrl->updateControl();
        vTaskDelay(delay);
    }
}

void uiTask(void *param) {
    auto ui = reinterpret_cast<UI*>(param);
    const TickType_t delay = pdMS_TO_TICKS(100);
    for (;;) {
        ui->updateUI();
        vTaskDelay(delay);
    }
}

void cloudTask(void *param) {
    auto cloud = reinterpret_cast<CloudClient*>(param);
    const TickType_t delay = pdMS_TO_TICKS(15000);
    for (;;) {
        cloud->uploadData();
        cloud->checkCommands();
        vTaskDelay(delay);
    }
}
