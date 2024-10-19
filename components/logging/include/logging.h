#pragma once

#include <esp_log.h>

#include <stdlib.h>
#include <stdio.h>

#ifdef DEBUG
  #undef DEBUG
  #define DEBUG 1
#else
  #define DEBUG 0
#endif

#if CONFIG_LOG_COLORS
  #undef  LOG_COLOR_D
  #define LOG_COLOR_D LOG_COLOR(LOG_COLOR_BLUE)
#endif

/* Bypass stdio buffering/interrupts, blocking write directly to UART */
#define LOG_ISR_ERROR(fmt, ...)   do { if (DEBUG) esp_rom_printf(LOG_FORMAT(E, fmt), esp_log_timestamp(), __func__, ##__VA_ARGS__); } while(0)
#define LOG_ISR_WARN(fmt, ...)    do { if (DEBUG) esp_rom_printf(LOG_FORMAT(W, fmt), esp_log_timestamp(), __func__, ##__VA_ARGS__); } while(0)
#define LOG_ISR_INFO(fmt, ...)    do { if (DEBUG) esp_rom_printf(LOG_FORMAT(I, fmt), esp_log_timestamp(), __func__, ##__VA_ARGS__); } while(0)
#define LOG_ISR_DEBUG(fmt, ...)   do { if (DEBUG) esp_rom_printf(LOG_FORMAT(D, fmt), esp_log_timestamp(), __func__, ##__VA_ARGS__); } while(0)

/* Bypass stdio buffering/interrupts, blocking write directly to UART */
#define LOG_BOOT_ERROR(fmt, ...)    do { esp_rom_printf(LOG_FORMAT(E, fmt), esp_log_timestamp(), __func__, ##__VA_ARGS__); } while(0)
#define LOG_BOOT_WARN(fmt, ...)     do { esp_rom_printf(LOG_FORMAT(W, fmt), esp_log_timestamp(), __func__, ##__VA_ARGS__); } while(0)
#define LOG_BOOT_INFO(fmt, ...)     do { esp_rom_printf(LOG_FORMAT(I, fmt), esp_log_timestamp(), __func__, ##__VA_ARGS__); } while(0)
#define LOG_BOOT_DEBUG(fmt, ...)    do { if (DEBUG) esp_rom_printf(LOG_FORMAT(D, fmt), esp_log_timestamp(), __func__, ##__VA_ARGS__); } while(0)

/* Use stdio stderr for logging, bypass esp_log tag levels */
#define LOG_FATAL(fmt, ...)     do { fprintf(stderr, LOG_FORMAT(E, fmt), esp_log_timestamp(), __func__, ##__VA_ARGS__); abort(); } while(0)
#define LOG_ERROR(fmt, ...)     do { fprintf(stderr, LOG_FORMAT(E, fmt), esp_log_timestamp(), __func__, ##__VA_ARGS__); } while(0)
#define LOG_WARN(fmt, ...)      do { fprintf(stderr, LOG_FORMAT(W, fmt), esp_log_timestamp(), __func__, ##__VA_ARGS__); } while(0)
#define LOG_INFO(fmt, ...)      do { fprintf(stderr, LOG_FORMAT(I, fmt), esp_log_timestamp(), __func__, ##__VA_ARGS__); } while(0)
#define LOG_DEBUG(fmt, ...)     do { if (DEBUG) fprintf(stderr, LOG_FORMAT(D, fmt), esp_log_timestamp(), __func__, ##__VA_ARGS__); } while(0)

// XXX: CONFIG_LOG_DEFAULT_LEVEL defaults to skip ESP_LOG_DEBUG, and raising will include ALL debug output by default - not possible to override this per-call
#if DEBUG
  #define LOG_DEBUG_BUFFER(buf, len)      IF_DEBUG({ ESP_LOG_BUFFER_HEX_LEVEL(__func__, buf, len, ESP_LOG_INFO); })
#else
  #define LOG_DEBUG_BUFFER(buf, len)
#endif
#define LOG_INFO_BUFFER(buf, len)      ESP_LOG_BUFFER_HEX_LEVEL(__func__, buf, len, ESP_LOG_INFO);
