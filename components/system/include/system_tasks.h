#ifndef __SYSTEM_TASKS_H__
#define __SYSTEM_TASKS_H__

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

char system_task_state_char(eTaskState state);
const char *system_task_state_str(eTaskState state);

#endif
