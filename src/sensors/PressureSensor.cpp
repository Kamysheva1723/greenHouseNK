#include "PressureSensor.h"
#include "PicoI2C.h" // so we have the full definition of PicoI2C
#include <cstdio>
#include <cmath>

PressureSensor::PressureSensor(std::shared_ptr<PicoI2C> my_i2c,
                               uint8_t i2cAddress,
                               float altitude_m,
                               bool useCrc)
        : my_i2c(my_i2c)
        , address(i2cAddress)
        , pressurePa(0.0f)
        , altitudeMeters(altitude_m)
        , crcEnabled(useCrc)
{
    // constructor body
}

bool PressureSensor::readSensor() {
    if (!my_i2c) {
        printf("\n[PressureSensor]: No I2C bus\n");
        return false;
    }

    // Example usage
    uint8_t command = 0xF1;
    int written = my_i2c->write(address, &command, 1);
    if (written != 1) {
        printf("PressureSensor: I2C write error\n");
        return false;
    }

    uint8_t buf[2];
    int readBytes = my_i2c->read(address, buf, 2);
    if (readBytes != 2) {
        printf("PressureSensor: I2C read error\n");
        return false;
    }

    // parse the data, etc.
    int16_t rawVal = (buf[0] << 8) | buf[1];
    pressurePa = rawVal / 60.0f;
    printf("\n[PressureSensor] rawVal=%d => %.2f Pa\n", rawVal, pressurePa);

    return true;
}

float PressureSensor::getValue() {
    // ISensor interface function
    return pressurePa;
}

float PressureSensor::getPressurePa() const {
    return pressurePa;
}
