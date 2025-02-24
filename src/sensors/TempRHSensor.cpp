#include "TempRHSensor.h"
#include "ModbusRegister.h"
#include <cstdio>
#include <utility>

TempRHSensor::TempRHSensor(std::shared_ptr<ModbusRegister> tempReg, std::shared_ptr<ModbusRegister> rhReg, std::shared_ptr<ModbusRegister> trhErrorReg)
        : temperature(0.0f), humidity(0.0f), treg(std::move(tempReg)), rhreg(std::move(rhReg)), trh_error_reg(std::move(trhErrorReg))
{}

bool TempRHSensor::readSensor() {
    if (!treg || !rhreg || !trh_error_reg) {
        std::printf("[TRHSensor] ERROR: Invalid registers.\n");
        return false;
    }

    constexpr int MAX_ATTEMPTS = 5;
    bool success = false;

    for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
        // 1) Read the error/status register
        uint16_t status = trh_error_reg->read();
        std::printf("[TRHSensor] Status = 0x%04X (attempt %d)\n", status, attempt + 1);

        // 2) Always read the raw temperature and humidity,
        //    but only *use* them if status is OK (==1).
        int raw_temp = treg->read();
        int raw_humidity = rhreg->read();
        std::printf("[TRHSensor] Raw T=%d, Raw RH=%d (tentative)\n",
                    raw_temp, raw_humidity);

        // 3) Check if status is "OK" (1). If so, accept the reading.
        if (status == 1) {
            temperature = static_cast<float>(raw_temp) / 10.0f;
            humidity    = static_cast<float>(raw_humidity) / 10.0f;

            std::printf("[TRHSensor] Accepting new values: "
                        "Temp=%.2f, RH=%.2f\n", temperature, humidity);

            success = true;
            break;  // Done
        } else {
            std::printf("[TRHSensor] WARNING: Status not OK (0x%04X). Retrying...\n", status);
        }

        // 4) A small delay before retrying
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    if (!success) {
        // We keep the previous temperature/humidity, print an error:
        std::printf("[TRHSensor] ERROR: No valid reading after %d attempts; "
                    "keeping old values: Temp=%.2f, RH=%.2f\n",
                    MAX_ATTEMPTS, temperature, humidity);
    }

    return success;
}


float TempRHSensor::getTemperature() const {
    return temperature;
}

float TempRHSensor::getHumidity() const {
    return humidity;
}
