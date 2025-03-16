#include <string>
#include <vector>
#include <iostream>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "rot/call_back.cpp"       // Contains the rotary encoder ISR callback implementation
#include "main.h"

// Define macro for recursive mutex support (ensures tasks can safely re-lock the same mutex)
#define configUSE_RECURSIVE_MUTEXES 1

// Wi-Fi credentials for remote connectivity
#define WIFI_SSID "SmartIotMQTT"
#define WIFI_PASSWORD "SmartIot"

// Uncomment to support HTTPS (TLS-based communication) if needed
#define CPPHTTPLIB_OPENSSL_SUPPORT  // Uncomment if you need HTTPS support.

// Project headers for various modules

#include "systemTasks/systemTasks.h"  // Declares tasks like sensorTask, uiTask, cloudTask, etc.
#include "./Controller/Controller.h"  // Contains Controller class for central decision-making
#include "UI/ui.h"                    // Declares the UI class for OLED display and rotary encoder
#include "cloud/cloud.h"              // Contains Cloud class for secure remote communication
#include "PicoOsUart.h"               // Wrapper for UART operations on Pico board (used by Modbus)
#include "ssd1306os.h"                // Driver for the SSD1306 OLED display over I2C
#include "PicoI2C.h"                  // Abstraction for I2C communication on the Pico
#include "./sensors/CO2Sensor.h"      // Sensor driver for CO₂ sensor via Modbus RTU
#include "./sensors/TempRHSensor.h"   // Sensor driver for temperature and humidity sensor
#include "./sensors/PressureSensor.h" // Sensor driver for differential pressure sensor via I2C
#include "./sensors/ISensor.h"        // Common interface for all sensors
#include "./FanDriver/FanDriver.h"    // Driver for fan control via Modbus register
#include "./ValveDriver/ValveDriver.h"// Driver for CO₂ valve control using GPIO
#include "EEPROM/EEPROMStorage.h"     // Driver for external EEPROM storage (for persisting setpoints)
#include "ModbusClient.h"             // Provides Modbus RTU client functionality over UART
#include "ModbusRegister.h"           // Represents a Modbus register for sensor/actuator data
#include "systemTasks/init-data.h."   // Global initialization structure definition
#include "IPStack.h"                  // Provides network stack abstractions
#include "cloud/cloud.h"              // Duplicate include; ensures Cloud module is defined (may be guarded)
#include "Controller/Controller.h"    // Duplicate include; ensures Controller module is defined 
#include "FreeRTOS.h"
#include "task.h"
#include "rot/GpioEvent.h"            // Defines structure for GPIO events from rotary encoder interrupts

// Global initialization data structure shared among tasks
InitDataStruct g_initData;
// Global FreeRTOS queue handle for GPIO events (e.g., rotation plus button press events)
QueueHandle_t xGpioQueue = nullptr;

// Forward declaration for FreeRTOS: runtime counter for profiling tasks
extern "C" {
uint32_t read_runtime_ctr(void) {
    // Return the current value of the hardware timer (used for run time stats)
    return timer_hw->timerawl;
}
}

/**
 * setupTask:
 * -----------
 * This task is created during system boot and is responsible for:
 *   - Initializing hardware interfaces (Wi-Fi, I2C, UART, GPIO pins)
 *   - Setting up the peripherals and instantiating all system objects (sensors, actuators, display, EEPROM, etc.)
 *   - Creating all other FreeRTOS tasks such as sensorTask, uiTask, cloudTask, etc.
 *   - Deleting itself once the system is fully set up.
 */
static void setupTask(void *param) {

    printf("SetupTask started in task: %s\n", pcTaskGetName(nullptr));

    // Initialize the Wi-Fi chip (cyw43) for connecting to the network
    if (cyw43_arch_init()) {
        printf("Failed to initialize cyw43_arch!\n");
        while (true) tight_loop_contents();
    }
    // Enable Wi-Fi STA (station) mode for client connectivity
    cyw43_arch_enable_sta_mode();

    // Connect to Wi-Fi with a timeout of 30 seconds.
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("WiFi connected!\n");
    } else {
        printf("Failed to connect to WiFi!\n");
        // Optionally add code to handle connection failure (retry logic, error reporting, etc.)
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Rotary Encoder Initialization
    ///////////////////////////////////////////////////////////////////////////////

    // 1) Create a FreeRTOS queue for rotary encoder GPIO events
    xGpioQueue = xQueueCreate(32, sizeof(GpioEvent));
    if (!xGpioQueue) {
        printf("Error creating xGpioQueue!\n");
        while(true) { tight_loop_contents(); }
    }

    // 2) Initialize rotary encoder GPIO pins:
    // Configure ROT_A_PIN as an input (primary signal for encoder rotation)
    gpio_init(ROT_A_PIN);
    gpio_set_dir(ROT_A_PIN, GPIO_IN);
    gpio_set_pulls(ROT_A_PIN, false, false);

    // Configure ROT_B_PIN as an input (complementary signal to determine rotation direction)
    gpio_init(ROT_B_PIN);
    gpio_set_dir(ROT_B_PIN, GPIO_IN);
    gpio_set_pulls(ROT_B_PIN, false, false);

    // Configure ROT_SW_PIN as an input with a pull-up resistor for button press events
    gpio_init(ROT_SW_PIN);
    gpio_set_dir(ROT_SW_PIN, GPIO_IN);
    gpio_pull_up(ROT_SW_PIN);

    // Attach interrupt service routines (ISRs) to capture rotary events:
    // - For the primary encoder pin (ROT_A_PIN), trigger on rising edge.
    gpio_set_irq_enabled_with_callback(ROT_A_PIN, GPIO_IRQ_EDGE_RISE, true, gpio_isr_callback);
    // - For the encoder switch (ROT_SW_PIN), trigger on falling edge (button press).
    gpio_set_irq_enabled_with_callback(ROT_SW_PIN, GPIO_IRQ_EDGE_FALL, true, gpio_isr_callback);

    ///////////////////////////////////////////////////////////////////////////////
    // I2C and UART Setup for sensors, display and EEPROM storage.
    ///////////////////////////////////////////////////////////////////////////////

    // 1) Initialize I2C instance for EEPROM:
    // Use the Pico SDK's global I2C instance 'i2c0' at 400kHz
    i2c_inst_t* eepromI2C = i2c0;
    i2c_init(eepromI2C, 400000);

    // Create an I2C object to be used by the display and pressure sensor
    auto i2cDispPres = std::make_shared<PicoI2C>(1, 400000);  // Using I2C bus instance 1

    // 2) Setup UART for Modbus RTU communications used by sensors (CO₂, Temp/RH) and fan driver.
    //    UART instance: 1, TX on GPIO 4, RX on GPIO 5, baud rate 9600, with a timeout of 2 (unit depends on implementation)
    auto my_uart1 = std::make_shared<PicoOsUart>(1, 4, 5, 9600, 2);

    // 3) Create a ModbusClient object that interfaces with the UART.
    auto rtu_client = std::make_shared<ModbusClient>(my_uart1);

    ///////////////////////////////////////////////////////////////////////////////
    // Sensor and Actuator Register Setup (Modbus registers for CO₂ sensor and HMP60 sensor)
    ///////////////////////////////////////////////////////////////////////////////

    // Create registers for CO₂ sensor:
    // a) co2LowReg: Holds the low word of the 16-bit CO₂ measurement (addressed at device 240, offset 256)
    auto co2LowReg     = std::make_shared<ModbusRegister>(rtu_client, 240, 256);
    // b) deviceStatus: Read sensor device status (address 240, offset 2048)
    auto deviceStatus  = std::make_shared<ModbusRegister>(rtu_client, 240, 2048);
    // c) co2Status: Read CO₂ specific status (address 240, offset 2049)
    auto co2Status     = std::make_shared<ModbusRegister>(rtu_client, 240, 2049);

    // Instantiate the CO₂ sensor object using the created registers;
    // this object implements the ISensor interface.
    auto co2Sensor = std::make_shared<CO2Sensor>(
            co2LowReg,
            deviceStatus,
            co2Status
    );

    // Create registers for HMP60 Temperature and Humidity sensor (device address 241)
    // a) Temperature reading register at offset 257
    auto temp_reg = std::make_shared<ModbusRegister>(rtu_client, 241, 257);
    // b) Relative Humidity register at offset 256
    auto rh_reg   = std::make_shared<ModbusRegister>(rtu_client, 241, 256);
    // c) Error/status register at offset 512 for sensor validation
    auto trh_error_reg   = std::make_shared<ModbusRegister>(rtu_client, 241, 512);

    // Instantiate the TempRHSensor object for temperature and humidity readings.
    auto thrSensor = std::make_shared<TempRHSensor>(temp_reg, rh_reg, trh_error_reg);

    // Create pressure sensor object for a differential pressure sensor via I2C.
    // I2C address for the sensor is 0x40.
    auto presSensor = std::make_shared<PressureSensor>(i2cDispPres, 0x40);

    ///////////////////////////////////////////////////////////////////////////////
    // Display, EEPROM, Fan and Valve Setup
    ///////////////////////////////////////////////////////////////////////////////

    // Create the SSD1306 OLED display object using the I2C instance for display,
    // with I2C address 0x3C and a display resolution of 128x64 pixels.
    auto display = std::make_shared<ssd1306os>(i2cDispPres, 0x3C, 128, 64);

    // Initialize EEPROM storage using I2C0, with designated SDA and SCL pins, and the device's I2C address.
    auto eepromStore = std::make_shared<EEPROMStorage>(eepromI2C, EEPROM_SDA_PIN, EEPROM_SCL_PIN, EEPROM_DEVICE_ADDRESS);

    // Create a Modbus register for the Fan Driver at device address 1, register offset 0.
    auto produal_reg = std::make_shared<ModbusRegister>(rtu_client, 1, 0);
    // Instantiate the FanDriver to control fan speed via Modbus.
    auto fanDriver = std::make_shared<FanDriver>(produal_reg);
    // Set the fan speed initially to 75%
    fanDriver->setFanSpeed(75.0f);

    // Create the ValveDriver to control the CO₂ valve using a GPIO pin (GPIO 27).
    auto valveDriver = std::make_shared<ValveDriver>(27);

    // Instantiate the main Controller object that aggregates sensor data and controls actuators.
    auto controller = std::make_shared<Controller>(co2Sensor, thrSensor, presSensor, fanDriver, valveDriver, eepromStore);

    // Create the Cloud module instance for remote cloud communications, passing the controller pointer.
    auto cloud = new Cloud(controller.get());

    // Instantiate the UI module to handle updating the OLED display and processing rotary encoder inputs.
    auto ui = std::make_shared<UI>(display, controller);

    // Create a dynamic sensor list for use by the sensorTask.
    auto *sensorList = new std::vector<std::shared_ptr<ISensor>>();
    sensorList->push_back(co2Sensor);
    sensorList->push_back(thrSensor);
    sensorList->push_back(presSensor);

    // Populate global initialization data structure so that other tasks can access shared objects.
    g_initData.eepromStore = eepromStore;
    g_initData.controller  = controller;
    g_initData.ui          = ui;
    g_initData.sensorList  = sensorList;

    ///////////////////////////////////////////////////////////////////////////////
    // Create FreeRTOS Tasks for various functionalities
    ///////////////////////////////////////////////////////////////////////////////

    // Create rotaryEventTask to process asynchronous rotary encoder events from the ISR.
    xTaskCreate(rotaryEventTask, "RotaryEventTask", 256, ui.get(), tskIDLE_PRIORITY+1, nullptr);
    // Create cloudTask to handle secure TLS communications for remote data reporting and command updates.
    xTaskCreate(cloudTask, "cloudTask",  2048, cloud, tskIDLE_PRIORITY+1, nullptr);
    // Create initTask to load stored EEPROM data (e.g., CO₂ setpoint) and initialize Controller/UI.
    xTaskCreate(initTask,   "InitTask",   1024, &g_initData,    tskIDLE_PRIORITY+3, nullptr);
    // Create eepromTask to handle periodic EEPROM operations for persistence.
    xTaskCreate(eepromTask, "EepromTask", 256,  eepromStore.get(), tskIDLE_PRIORITY+1, nullptr);
    // Create sensorTask to periodically read sensor data and update the Controller.
    xTaskCreate(sensorTask, "SensorTask", 512,  &g_initData,     tskIDLE_PRIORITY+1, nullptr);
    // Create uiTask to manage the OLED display and local user interactions.
    xTaskCreate(uiTask,     "UITask",     256,  ui.get(),       tskIDLE_PRIORITY+1, nullptr);

    // All tasks are created successfully; delete this setup task as its job is done.
    printf("SetupTask: All tasks created. Deleting SetupTask...\n");
    vTaskDelete(nullptr);
}

/**
 * main:
 * -----
 * Entry point for the Greenhouse Fertilization System firmware.
 * Initializes standard I/O, creates the setupTask and then starts the FreeRTOS scheduler.
 */
int main() {
    // Initialize standard I/O (needed for printf over UART/USB)
    stdio_init_all();
    printf("==== Greenhouse Controller Boot ====\n");

    // Create the setupTask with a higher priority to ensure system initialization occurs before any dependent tasks.
    xTaskCreate(setupTask, "SetupTask", 1024, nullptr, tskIDLE_PRIORITY+3, nullptr);

    // Start the FreeRTOS scheduler; this never returns.
    vTaskStartScheduler();

    // If the scheduler ever exits, enter an infinite loop.
    while (true) {
        tight_loop_contents();
    }
    return 0;
}