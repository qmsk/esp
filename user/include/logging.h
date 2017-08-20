#ifndef __USER_LOGGING_H__
#define __USER_LOGGING_H__

#include "user_config.h"

void logging_printf(const char *prefix, const char *func, const char *fmt, ...);

#ifdef DEBUG
  #define LOG_DEBUG(...)  do { logging_printf("DEBUG ", __func__, __VA_ARGS__); } while(0)
#else
  #define LOG_DEBUG(...)
#endif

#define LOG_INFO(...)   do { logging_printf("INFO  ", __func__, __VA_ARGS__); } while(0)
#define LOG_WARN(...)   do { logging_printf("WARN  ", __func__, __VA_ARGS__); } while(0)
#define LOG_ERROR(...)  do { logging_printf("ERROR ", __func__, __VA_ARGS__); } while(0)

int init_logging(struct user_config *config);

#endif
