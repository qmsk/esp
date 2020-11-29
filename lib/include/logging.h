#ifndef __LIB_LOGGING_H__
#define __LIB_LOGGING_H__

void logging_printf(const char *prefix, const char *func, const char *fmt, ...)
  __attribute((format (printf, 3, 4)));

#ifdef DEBUG
  #define LOG_DEBUG(...)  do { logging_printf("DEBUG ", __func__, __VA_ARGS__); } while(0)
#else
  #define LOG_DEBUG(...)
#endif

#define LOG_INFO(...)   do { logging_printf("INFO  ", __func__, __VA_ARGS__); } while(0)
#define LOG_WARN(...)   do { logging_printf("WARN  ", __func__, __VA_ARGS__); } while(0)
#define LOG_ERROR(...)  do { logging_printf("ERROR ", __func__, __VA_ARGS__); } while(0)

int logging_init();

#endif
