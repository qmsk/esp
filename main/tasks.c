#include "tasks.h"

#include <logging.h>

#include <stdarg.h>

#if CONFIG_IDF_TARGET_ESP8266
  int start_task(struct task_options options)
  {
    if (xTaskCreate(
      options.main,
      options.name,
      options.stack_size,
      options.arg,
      options.priority,
      options.handle
    ) <= 0) {
      return -1;
    }

    return 0;
  }
#elif CONFIG_IDF_TARGET_ESP32
  int start_task(struct task_options options)
  {
    if (xTaskCreatePinnedToCore(
      options.main,
      options.name,
      options.stack_size,
      options.arg,
      options.priority,
      options.handle,
      options.affinity
    ) <= 0) {
      return -1;
    }

    return 0;
  }
#endif

int start_taskf(struct task_options options, ...)
{
  va_list args;
  int ret;

  va_start(args, options);
  ret = vsnprintf(options.name, sizeof(options.name), options.name_fmt, args);
  va_end(args);

  if (ret >= sizeof(options.name)) {
    LOG_ERROR("snprintf: task name overflow");
    return -1;
  }

  return start_task(options);
}
