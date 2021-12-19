#include "pin_mutex.h"

SemaphoreHandle_t pin_mutex[PIN_MUTEX_MAX] = {};

int init_pin_mutex()
{
  for (enum pin_mutex m = 0; m < PIN_MUTEX_MAX; m++) {
    if (!(pin_mutex[m] = xSemaphoreCreateMutex())) {
      return -1;
    }
  }

  return 0;
}
