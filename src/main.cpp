//
// Created by natal on 26.1.2025.
//
#include "systemTasks.h"
#include "Sensors.h"
#include "FanDriver.h"
#include "ValveDriver.h"
#include "Controller.h"
#include "EEPROM.h"
#include "UI.h"
#include "cloud.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include "gpio.h"

int main() {
    stdio_init_all();
    printf("Greenhouse Controller Boot\n");

    // Setup I2C & UART drivers
    auto i2c0 = std::make_shared<PicoI2C>(0, 100000);
    auto uart1 = std::make_shared<PicoOsUart>(1, 4, 5, 9600, 2);

    // Create sensors
    auto co2Sensor   = std::make_shared<CO2Sensor>(uart1, 240);
    auto thrSensor   = std::make_shared<TempRHSensor>(uart1, 241);
    auto presSensor  = std::make_shared<PressureSensor>(i2c0, 0x40);

    // Create fan driver & valve
    auto fanDriver   = std::make_shared<FanDriver>(uart1, 1);
    auto valveDriver = std::make_shared<ValveDriver>(27 /* GPIO pin */);

    // Create controller
    auto controller  = std::make_shared<Controller>(co2Sensor, thrSensor, presSensor,
                                                    fanDriver, valveDriver);

    // Create EEPROM storage
    auto eepromStore = std::make_shared<EEPROMStorage>(i2c0, 0x50);

    // Load last stored setpoint
    uint16_t sp = eepromStore->loadCO2Setpoint();
    controller->setCO2Setpoint(sp);

    // Create UI (assuming you have an ssd1306os driver object)
    auto display = std::make_shared<ssd1306os>(i2c0);
    auto ui = std::make_shared<UI>(display, eepromStore, controller);

    // Create cloud client
    auto cloudClient = std::make_shared<CloudClient>("THINGSPEAK_API_KEY",
                                                     "TALKBACK_KEY",
                                                     "CHANNEL_ID");
    cloudClient->setController(controller);

    // Create tasks
    static std::vector<ISensor*> sensorList{ co2Sensor.get(), thrSensor.get(), presSensor.get() };
    xTaskCreate(sensorTask,  "SensorTask", 512, &sensorList,    1, nullptr);
    xTaskCreate(controlTask, "ControlTask",512, controller.get(),1, nullptr);
    xTaskCreate(uiTask,      "UITask",     512, ui.get(),       1, nullptr);
    xTaskCreate(cloudTask,   "CloudTask",  1024,cloudClient.get(),1,nullptr);

    vTaskStartScheduler();

    while(true) {}
    return 0;
}
