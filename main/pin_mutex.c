#include "pin_mutex.h"

SemaphoreHandle_t pin_mutex[PIN_MUTEX_MAX] = {};

int init_pin_mutex()
{
  for (enum pin_mutex m = 0; m < PIN_MUTEX_MAX; m++) {
    // use counting mutex initialized as available for `xSemaphoreTake()`.
    if (!(pin_mutex[m] = xSemaphoreCreateCounting(1, 1))) {
      return -1;
    }
  }

  return 0;
}
