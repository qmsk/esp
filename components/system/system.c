#include <system.h>

#include <esp_heap_caps.h>
#include <esp_idf_version.h>
#include <esp_system.h>
#include <esp_timer.h>

#if CONFIG_IDF_TARGET_ESP8266
# define IDF_NAME "ESP8266_RTOS_SDK"

// compat, same value used by system_api.c and newlib esp_malloc.c
# define MALLOC_CAP_DEFAULT (MALLOC_CAP_32BIT)

size_t heap_caps_get_total_size(uint32_t caps);

# include <esp_clk.h>

#elif CONFIG_IDF_TARGET_ESP32
# define IDF_NAME "esp-idf"

# include <esp32/clk.h>

#endif
/* Crude tracking of heap size at boot */
static size_t maximum_free_heap_size = 0;

size_t system_get_total_heap_size()
{
  return heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
}

size_t system_get_free_heap_size()
{
  return heap_caps_get_free_size( MALLOC_CAP_DEFAULT );
}

size_t system_get_minimum_free_heap_size()
{
  return heap_caps_get_free_size( MALLOC_CAP_DEFAULT );
}

size_t system_get_maximum_free_heap_size()
{
  return maximum_free_heap_size;
}

void system_update_maximum_free_heap_size()
{
  size_t free_heap_size = esp_get_free_heap_size();

  if (free_heap_size > maximum_free_heap_size) {
    maximum_free_heap_size = free_heap_size;
  }
}

void system_info_get(struct system_info *info)
{
  esp_chip_info(&info->esp_chip_info);

  info->esp_app_desc = esp_ota_get_app_description();

  info->esp_idf_name = IDF_NAME;
  info->esp_idf_version = esp_get_idf_version();
}

void system_status_get(struct system_status *status)
{
  int64_t uptime = esp_timer_get_time();

  status->reset_reason = esp_reset_reason();
  status->uptime_s = (uptime / 1000000);
  status->uptime_us = (uptime % 1000000);
  status->cpu_frequency = esp_clk_cpu_freq();
  status->total_heap_size = system_get_total_heap_size();
  status->free_heap_size = system_get_free_heap_size();
  status->minimum_free_heap_size = system_get_minimum_free_heap_size();
  status->maximum_free_heap_size = system_get_maximum_free_heap_size();

  // ESP32 TODO: heap_caps_get_info() for more detailed DRAM/IRAM numbers?
}
