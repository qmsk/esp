#undef _GNU_SOURCE
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <sys/features.h>

#if __XSI_VISIBLE >= 4 && __POSIX_VISIBLE < 200112
#else
# error wut
#endif

#include <json.h>
#include "write.h"

#include <stdlib.h>

#include <sdkconfig.h>

int json_write_float(struct json_writer *w, float value)
{
#if CONFIG_NEWLIB_NANO_FORMAT && CONFIG_IDF_TARGET_ESP8266
  // ESP8266_RTOS_SDK newlib-nano does not support float
  char buf[32];

  return json_writef(w, JSON_NUMBER, "%s", gcvtf(value, 6, buf));

#else
  // esp-idf newlib-nano supports floats
  return json_writef(w, JSON_NUMBER, "%f", value);
#endif
}
