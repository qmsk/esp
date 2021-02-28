#ifndef __LOGGING_H__
#define __LOGGING_H__

#include "esp_log.h"

#ifdef DEBUG
  #define LOG_DEBUG(...)  do { esp_log_write(ESP_LOG_DEBUG, __func__, __VA_ARGS__); } while(0)
#else
  #define LOG_DEBUG(...)
#endif

#define LOG_INFO(...)   do { esp_log_write(ESP_LOG_INFO, __func__, __VA_ARGS__); } while(0)
#define LOG_WARN(...)   do { esp_log_write(ESP_LOG_WARN, __func__, __VA_ARGS__); } while(0)
#define LOG_ERROR(...)  do { esp_log_write(ESP_LOG_ERROR, __func__, __VA_ARGS__); } while(0)

#endif
