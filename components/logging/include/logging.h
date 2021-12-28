#ifndef __LOGGING_H__
#define __LOGGING_H__

#include "esp_log.h"

/* Bypass stdio */
#define LOG_BOOT_INFO(...)    ESP_EARLY_LOGI(__func__, __VA_ARGS__)
#define LOG_BOOT_ERROR(...)   ESP_EARLY_LOGE(__func__, __VA_ARGS__)

#ifdef DEBUG
  #define LOG_DEBUG(...)      ESP_EARLY_LOGD(__func__, __VA_ARGS__)
  #define LOG_BOOT_DEBUG(...) ESP_EARLY_LOGD(__func__, __VA_ARGS__)
  #define LOG_ISR_DEBUG(...)  ESP_EARLY_LOGD(__func__, __VA_ARGS__)
#else
  #define LOG_DEBUG(...)
  #define LOG_BOOT_DEBUG(...)
  #define LOG_ISR_DEBUG(...)
#endif

#define LOG_INFO(...)   ESP_LOG_LEVEL(ESP_LOG_INFO, __func__, __VA_ARGS__)
#define LOG_WARN(...)   ESP_LOG_LEVEL(ESP_LOG_WARN, __func__, __VA_ARGS__)
#define LOG_ERROR(...)  ESP_LOG_LEVEL(ESP_LOG_ERROR, __func__, __VA_ARGS__)

#endif
