#ifndef SYSTEM_TASKS_H
#define SYSTEM_TASKS_H

#include <cstdint>

struct InitDataStruct;

// Existing tasks:
void initTask(void* param);
void sensorTask(void* param);
void controlTask(void* param);
void uiTask(void* param);
void eepromTask(void* param);
void rotaryEventTask(void *param);

// NEW: The cloud task prototype
void cloudTask(void* param);

#endif // SYSTEM_TASKS_H

