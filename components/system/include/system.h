#pragma once

#include <esp_system.h>
#include <esp_ota_ops.h>

#include <stddef.h>

const char *esp_reset_reason_str(esp_reset_reason_t reason);
const char *esp_chip_model_str(esp_chip_model_t model);

struct system_image_info {
  size_t iram_total_size, dram_total_size;

  unsigned irom_start, iram_start, dram_start, drom_start;
  unsigned irom_end, iram_end, dram_end, drom_end;
  size_t irom_size, iram_size, dram_size, drom_size;

  unsigned iram_heap_start, dram_heap_start;
  unsigned iram_heap_end, dram_heap_end;
  size_t iram_heap_size, dram_heap_size;

  size_t flash_size, flash_usage;
};

/*
 * Static info from system image
 */
struct system_info {
    esp_chip_info_t esp_chip_info;

    const esp_app_desc_t *esp_app_desc;

    const char *esp_idf_name;
    const char* esp_idf_version;
};

/*
 * Dynamic info from system state
 */
struct system_status {
  esp_reset_reason_t reset_reason;
  uint32_t uptime_s, uptime_us; // s + us
  int cpu_frequency; // gz

  size_t total_heap_size, free_heap_size, minimum_free_heap_size, maximum_free_heap_size;
};

void system_image_info_get(struct system_image_info *info);
void system_info_get(struct system_info *info);
void system_status_get(struct system_status *status);

/* Memory */
size_t system_get_total_heap_size();
size_t system_get_free_heap_size();
size_t system_get_minimum_free_heap_size();
size_t system_get_maximum_free_heap_size();

// used tgo set system_get_maximum_free_heap_size at boot
void system_update_maximum_free_heap_size();
