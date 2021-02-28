#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <stddef.h>
#include <esp_system.h>

typedef struct system_image_info {
  void *iram_start, *dram_start, *flash_start;
  void *iram_end, *dram_end, *flash_end;
  size_t iram_usage, dram_usage, flash_usage;
  size_t iram_size, dram_size;

  void *iram_heap_start, *dram_heap_start;
  size_t iram_heap_size, dram_heap_size;
} system_image_info_t;

void system_image_info(struct system_image_info *info);

size_t system_get_total_heap_size();

/* Return string for reset reason */
const char *esp_reset_reason_str(esp_reset_reason_t reason);

#endif
