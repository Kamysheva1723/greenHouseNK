//
// Created by natal on 14.2.2025.
//

#ifndef MHZTASK_H
#define MHZTASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

// Declare the sensor mhz19c task function.
void MHZTask(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // MHZTASK_H

