#ifndef TEMP_RH_SENSOR_H
#define TEMP_RH_SENSOR_H

#include <memory>
#include "ISensor.h"

class ModbusRegister;

class TempRHSensor : public ISensor {
public:
    TempRHSensor(std::shared_ptr<ModbusRegister> tempReg,
                 std::shared_ptr<ModbusRegister> rhReg,
                 std::shared_ptr<ModbusRegister> trhErrorReg);
    bool readSensor() override;
    float getTemperature() const;
    float getHumidity() const;
private:
    float temperature;
    float humidity;
    std::shared_ptr<ModbusRegister> treg;
    std::shared_ptr<ModbusRegister> rhreg;
    std::shared_ptr<ModbusRegister> trh_error_reg;
};

#endif // TEMP_RH_SENSOR_H

