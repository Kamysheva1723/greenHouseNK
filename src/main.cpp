#include <string>
#include <vector>
#include <iostream>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "rot/call_back.cpp"
#include "main.h"
#define configUSE_RECURSIVE_MUTEXES 1



#define CPPHTTPLIB_OPENSSL_SUPPORT  // Uncomment if you need HTTPS support.


// Project headers

#include "systemTasks/systemTasks.h"
#include "./Controller/Controller.h"
#include "UI/ui.h"
#include "cloud/cloud.h"
#include "PicoOsUart.h"
#include "ssd1306os.h"
#include "PicoI2C.h"
#include "./sensors/CO2Sensor.h"
#include "./sensors/TempRHSensor.h"
#include "./sensors/PressureSensor.h"
#include "./sensors/ISensor.h"
#include "./FanDriver/FanDriver.h"
#include "./ValveDriver/ValveDriver.h"
#include "EEPROM/EEPROMStorage.h"
#include "ModbusClient.h"
#include "ModbusRegister.h"
#include "systemTasks/init-data.h."
#include "IPStack.h"
#include "cloud/cloud.h"
#include "Controller/Controller.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rot/GpioEvent.h"
//#define WIFI_SSID "Koti_2951"
//#define WIFI_PASSWORD "GD34L8GHD4KYT"
#define WIFI_SSID "AndroidAP6AF5"
#define WIFI_PASSWORD "ojbd8927"




InitDataStruct g_initData;
QueueHandle_t xGpioQueue = nullptr;



// Forward declaration for FreeRTOS
extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}



/**
 * This task runs once the scheduler starts. It creates all our shared objects (I2C, UART,
 * sensors, etc.), then creates the "real" tasks (initTask, sensorTask, etc.), and finally
 * deletes itself.
 */
static void setupTask(void *param) {
    printf("SetupTask started in task: %s\n", pcTaskGetName(nullptr));



    // Initialize the cyw43 (Wi-Fi) system.
    if (cyw43_arch_init()) {
        printf("Failed to initialize cyw43_arch!\n");
        while (true) tight_loop_contents();
    }
    cyw43_arch_enable_sta_mode();

// Connect to the WiFi network (example timeout: 30 seconds)
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("WiFi connected!\n");
    } else {
        printf("Failed to connect to WiFi!\n");
        // Handle connection failure...
    }


    // 1) Create the rotary event queue
    xGpioQueue = xQueueCreate(32, sizeof(GpioEvent));
    if (!xGpioQueue) {
        printf("Error creating xGpioQueue!\n");
        while(true) { tight_loop_contents(); }
    }

    // 2) Initialize the pins for the rotary encoder
    gpio_init(ROT_A_PIN);
    gpio_set_dir(ROT_A_PIN, GPIO_IN);
    gpio_set_pulls(ROT_A_PIN, false, false);

    gpio_init(ROT_B_PIN);
    gpio_set_dir(ROT_B_PIN, GPIO_IN);
    gpio_set_pulls(ROT_B_PIN, false, false);

    gpio_init(ROT_SW_PIN);
    gpio_set_dir(ROT_SW_PIN, GPIO_IN);
    gpio_pull_up(ROT_SW_PIN);

    // Attach the ISR callback
    gpio_set_irq_enabled_with_callback(ROT_A_PIN, GPIO_IRQ_EDGE_RISE, true, gpio_isr_callback);
    gpio_set_irq_enabled_with_callback(ROT_SW_PIN, GPIO_IRQ_EDGE_FALL, true, gpio_isr_callback);



    // 1) Create I2C instances
    i2c_inst_t* eepromI2C = i2c0;  // Use the raw global instance provided by the Pico SDK
    i2c_init(eepromI2C, 400000);

    auto i2cDispPres = std::make_shared<PicoI2C>(1, 400000);  // For display + pressure sensor

    // 2) Create UART for Modbus RTU sensors + fan driver
    auto my_uart1 = std::make_shared<PicoOsUart>(1, 4, 5, 9600, 2);

    // 3) Create the ModbusClient (used by ModbusRegister, sensors, fan driver)
    auto rtu_client = std::make_shared<ModbusClient>(my_uart1);

    // Create registers with zero-based addresses:
    // For the 16-bit CO₂ measurement:
// Example instantiation (addresses shown in decimal 0-based):
    auto co2LowReg     = std::make_shared<ModbusRegister>(rtu_client, 240, 256); // 16 bit CO₂ float
    // MSW of the CO₂ float
    auto deviceStatus  = std::make_shared<ModbusRegister>(rtu_client, 240, 2048); // Device status
    auto co2Status     = std::make_shared<ModbusRegister>(rtu_client, 240, 2049); // CO₂ status

    auto co2Sensor = std::make_shared<CO2Sensor>(
            co2LowReg,
            deviceStatus,
            co2Status
    );

    // For HMP60 (temp + humidity) at address=241
    auto temp_reg = std::make_shared<ModbusRegister>(rtu_client, 241, 257);
    auto rh_reg   = std::make_shared<ModbusRegister>(rtu_client, 241, 256);
    auto trh_error_reg   = std::make_shared<ModbusRegister>(rtu_client, 241, 512);

    auto thrSensor = std::make_shared<TempRHSensor>(temp_reg, rh_reg, trh_error_reg);

    // For pressure sensor (I2C addr=0x40)
    auto presSensor = std::make_shared<PressureSensor>(i2cDispPres, 0x40);

    // 5) Create the display (SSD1306)
    auto display = std::make_shared<ssd1306os>(i2cDispPres, 0x3C, 128, 64);

    // 6) Create EEPROM storage on I2C0
    auto eepromStore = std::make_shared<EEPROMStorage>(eepromI2C, EEPROM_SDA_PIN, EEPROM_SCL_PIN, EEPROM_DEVICE_ADDRESS);

    // 7) Create fan driver (Modbus address=1)

    auto produal_reg = std::make_shared<ModbusRegister>(rtu_client, 1, 0);


    auto fanDriver = std::make_shared<FanDriver>(produal_reg);
    fanDriver->setFanSpeed(75.0f);

    // 8) Create valve driver (GPIO 27)
    auto valveDriver = std::make_shared<ValveDriver>(27);

    // 9) Create the main Controller
    auto controller = std::make_shared<Controller>(co2Sensor, thrSensor, presSensor, fanDriver, valveDriver, eepromStore);

    auto cloud = new Cloud(controller.get());

    // 11) Create UI
    auto ui = std::make_shared<UI>(display, controller);


    // 13) Create a dynamic sensor list for the sensorTask
    auto *sensorList = new std::vector<std::shared_ptr<ISensor>>();
    sensorList->push_back(co2Sensor);
    sensorList->push_back(thrSensor);
    sensorList->push_back(presSensor);

    // 12) Fill global init data
    g_initData.eepromStore = eepromStore;
    g_initData.controller  = controller;
    g_initData.ui          = ui;
    g_initData.sensorList = sensorList;

    // 14) Now create the tasks

    xTaskCreate(rotaryEventTask, "RotaryEventTask", 256, ui.get(), tskIDLE_PRIORITY+1, nullptr);
    xTaskCreate(cloudTask, "cloudTask",  2048, cloud, tskIDLE_PRIORITY+1, nullptr);
    xTaskCreate(initTask,   "InitTask",   1024, &g_initData,    tskIDLE_PRIORITY+3, nullptr);
    xTaskCreate(eepromTask, "EepromTask", 256,  eepromStore.get(), tskIDLE_PRIORITY+1, nullptr);
    xTaskCreate(sensorTask, "SensorTask", 512,  &g_initData,     tskIDLE_PRIORITY+1, nullptr);

    xTaskCreate(uiTask,     "UITask",     256,  ui.get(),       tskIDLE_PRIORITY+1, nullptr);


   printf("SetupTask: All tasks created. Deleting SetupTask...\n");
    vTaskDelete(nullptr); // Kill the setup task
}

int main() {
    stdio_init_all();
    printf("==== Greenhouse Controller Boot ====\n");




    // Create the setupTask with a higher priority so it runs before other tasks start
    xTaskCreate(setupTask, "SetupTask", 1024, nullptr, tskIDLE_PRIORITY+3, nullptr);

    // Start the scheduler
    vTaskStartScheduler();

    // We should never get here
    while (true) {
        tight_loop_contents();
    }
    return 0;
}
