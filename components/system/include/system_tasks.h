#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

char system_task_state_char(eTaskState state);
const char *system_task_state_str(eTaskState state);

struct system_tasks_state {
  TaskStatus_t *tasks;
  unsigned count;
  uint32_t total_runtime;
};

/*
 * Returns TaskStatus from state for task matching given status.
 *
 * @return NULL if no matching task
 */
const TaskStatus_t *system_tasks_find(const struct system_tasks_state *state, const TaskStatus_t *task);

/*
 * @return permille value, 0-1000
 */
uint32_t system_tasks_total_usage(const struct system_tasks_state *state, const TaskStatus_t *task);
uint32_t system_tasks_last_usage(const struct system_tasks_state *state, const TaskStatus_t *task, const struct system_tasks_state *last);

int system_tasks_update(struct system_tasks_state *state);

void system_tasks_release(struct system_tasks_state *state);
