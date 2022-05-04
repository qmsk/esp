#ifndef __LOGGING_H__
#define __LOGGING_H__

#include "esp_log.h"

#ifdef DEBUG
# define IF_DEBUG(x) do { if (1) x } while(0)
#else
# define IF_DEBUG(x) do { if (0) x } while(0)
#endif

#define LOG_ISR_DEBUG(...)  IF_DEBUG({ ESP_EARLY_LOGD(__func__, __VA_ARGS__); })
#define LOG_ISR_INFO(...)  IF_DEBUG({ ESP_EARLY_LOGI(__func__, __VA_ARGS__); })
#define LOG_ISR_WARN(...)  IF_DEBUG({ ESP_EARLY_LOGW(__func__, __VA_ARGS__); })

/* Bypass stdio */
#define LOG_BOOT_DEBUG(...)   IF_DEBUG({ ESP_EARLY_LOGD(__func__, __VA_ARGS__); })
#define LOG_BOOT_INFO(...)    ESP_EARLY_LOGI(__func__, __VA_ARGS__)
#define LOG_BOOT_ERROR(...)   ESP_EARLY_LOGE(__func__, __VA_ARGS__)

// XXX: CONFIG_LOG_DEFAULT_LEVEL defaults to skip ESP_LOG_DEBUG, and raising will include ALL debug output by default - not possible to override this per-call
#define LOG_DEBUG(...)      IF_DEBUG({ ESP_LOG_LEVEL(ESP_LOG_INFO, __func__, __VA_ARGS__); })
#define LOG_INFO(...)   ESP_LOG_LEVEL(ESP_LOG_INFO, __func__, __VA_ARGS__)
#define LOG_WARN(...)   ESP_LOG_LEVEL(ESP_LOG_WARN, __func__, __VA_ARGS__)
#define LOG_ERROR(...)  ESP_LOG_LEVEL(ESP_LOG_ERROR, __func__, __VA_ARGS__)

// TODO: fprint(stderr) + fflush(stderr) + fsync(stderr) + abort()?
#define LOG_FATAL(...)  do { ESP_LOG_LEVEL(ESP_LOG_ERROR, __func__, __VA_ARGS__); abort(); } while(0)

// XXX: CONFIG_LOG_DEFAULT_LEVEL defaults to skip ESP_LOG_DEBUG, and raising will include ALL debug output by default - not possible to override this per-call
#define LOG_DEBUG_BUFFER(buf, len)      IF_DEBUG({ ESP_LOG_BUFFER_HEX_LEVEL(__func__, buf, len, ESP_LOG_INFO); })
#define LOG_INFO_BUFFER(buf, len)      ESP_LOG_BUFFER_HEX_LEVEL(__func__, buf, len, ESP_LOG_INFO);

#endif
