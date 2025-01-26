//
// Created by natal on 26.1.2025.
//

#ifndef GREENHOUSE_SYSTEMTASK_H
#define GREENHOUSE_SYSTEMTASK_H

void sensorTask(void *param);
void controlTask(void *param);
void uiTask(void *param);
void cloudTask(void *param);

#endif //GREENHOUSE_SYSTEMTASK_H
