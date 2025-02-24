#ifndef INIT_DATA_H
#define INIT_DATA_H

#include <memory>
#include "EEPROM/EEPROMStorage.h"
#include "./Controller/Controller.h"
#include "UI/ui.h"

struct InitDataStruct {
    std::shared_ptr<EEPROMStorage> eepromStore;
    std::shared_ptr<Controller> controller;
    std::shared_ptr<UI> ui;
    std::vector<std::shared_ptr<ISensor>>* sensorList;
};

#endif // INIT_DATA_H
