#include "CO2Sensor.h"
#include <cstdio>      // for printf
#include <cstdint>
#include <cstring>     // for memcpy
#include "ModbusRegister.h"

// If your Modbus library already gives you each 16-bit register in host order,
// then do NOT call swapBytes here.
static inline uint16_t swapBytes(uint16_t val) {

    return (val >> 8) | (val << 8);

}

// A short FreeRTOS-based delay macro (adjust as needed)
static inline void shortDelayMs(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}


CO2Sensor::CO2Sensor(std::shared_ptr<ModbusRegister> co2LowReg,

                     std::shared_ptr<ModbusRegister> deviceStatusReg,
                     std::shared_ptr<ModbusRegister> co2StatusReg)
        : co2LowReg_(co2LowReg),
          deviceStatusReg_(deviceStatusReg),
          co2StatusReg_(co2StatusReg),
          co2ppm_(0.0f)
{
}

bool CO2Sensor::readSensor() {
    const int maxAttempts = 10;
    bool success = false;
    float newReading = 0.0f;

    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        // 1) Check Device Status (0 = OK)
        bool devOK = true;
        if (deviceStatusReg_) {
            uint16_t devStatus = deviceStatusReg_->read();
            std::printf("[CO2Sensor] Device Status = 0x%04X\n", devStatus);
            if (devStatus != 0) {
                std::printf("[CO2Sensor] WARNING: device status not OK (0x%04X), attempt %d\n",
                            devStatus, attempt + 1);
                devOK = false;
            }
        }

        // 2) Check CO₂ Status (0 = OK)
        bool co2OK = true;
        if (co2StatusReg_) {
            uint16_t status = co2StatusReg_->read();
            std::printf("[CO2Sensor] CO₂ Status = 0x%04X\n", status);
            if (status != 0) {
                std::printf("[CO2Sensor] WARNING: CO₂ status not OK (0x%04X), attempt %d\n",
                            status, attempt + 1);
                co2OK = false;
            }
        }

        // 3) Always read the raw register (so the UART buffer is consumed),
        //    but only *use* it if devOK & co2OK.
        if (!co2LowReg_) {
            std::printf("[CO2Sensor] ERROR: Null register pointer!\n");
            return false;
        }
        uint16_t raw = co2LowReg_->read();  // always read
        std::printf("[CO2Sensor] Raw=0x%04X => ~%.1f ppm (tentative)\n", raw, (float)raw);

        // 4) Store newReading *only* if devOK and co2OK
        if (devOK && co2OK) {
            newReading = static_cast<float>(raw);
            std::printf("[CO2Sensor] Accepting new CO₂ reading: %.1f ppm\n", newReading);

            // We succeeded in getting a "valid" reading
            success = true;
            break;
        }

        // If not OK, we discard 'raw' and go to next attempt.
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (success) {
        // Only update co2ppm_ if we found a valid reading.
        co2ppm_ = newReading;
    } else {
        // Keep old co2ppm_, report failure
        std::printf("[CO2Sensor] ERROR: No valid reading after %d attempts.\n", maxAttempts);
        return false;
    }

    // optional extra delay
    vTaskDelay(pdMS_TO_TICKS(3000));
    return true;
}


float CO2Sensor::getCO2ppm() const {
    return co2ppm_;
}

float CO2Sensor::getValue() {
    return getCO2ppm();
}

