#include <system_tasks.h>
#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stdlib.h>

char system_task_state_char(eTaskState state)
{
    switch(state) {
      case eRunning:    return 'X';
      case eReady:      return 'R';
      case eBlocked:    return 'B';
      case eSuspended:  return 'S';
      case eDeleted:    return 'D';
      case eInvalid:    return 'I';
      default:          return '?';
    }
}

const char *system_task_state_str(eTaskState state)
{
  switch(state) {
    case eRunning:    return "Running";
    case eReady:      return "Ready";
    case eBlocked:    return "Blocked";
    case eSuspended:  return "Suspended";
    case eDeleted:    return "Deleted";
    case eInvalid:    return "Invalid";
    default:          return "Unknown";
  }
}

const TaskStatus_t *system_tasks_find(const struct system_tasks_state *state, const TaskStatus_t *task)
{
  for (TaskStatus_t *t = state->tasks; t < state->tasks + state->count; t++) {
    if (t->xTaskNumber == task->xTaskNumber) {
      return t;
    }
  }

  return NULL;
}

uint32_t system_tasks_total_usage(const struct system_tasks_state *state, const TaskStatus_t *task)
{
  return task->ulRunTimeCounter / (state->total_runtime / 1000);
}

uint32_t system_tasks_last_usage(const struct system_tasks_state *state, const TaskStatus_t *task, const struct system_tasks_state *last_state)
{
  const TaskStatus_t *t;
  uint32_t last_runtime = state->total_runtime - last_state->total_runtime;

  if (!(t = system_tasks_find(last_state, task))) {
    return 0;
  }

  return (task->ulRunTimeCounter - t->ulRunTimeCounter) / (last_runtime / 1000);
}


int system_tasks_update(struct system_tasks_state *state)
{
  state->count = uxTaskGetNumberOfTasks();

  if (!(state->tasks = calloc(state->count, sizeof(*state->tasks)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if (!(state->count = uxTaskGetSystemState(state->tasks, state->count, &state->total_runtime))) {
    LOG_ERROR("uxTaskGetSystemState");
    goto error;
  }

  return 0;

error:
  free(state->tasks);

  return -1;
}

void system_tasks_release(struct system_tasks_state *state)
{
  free(state->tasks);

  state->tasks = NULL;
  state->count = 0;
  state->total_runtime = 0;
}
