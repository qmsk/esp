#include <system_tasks.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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
