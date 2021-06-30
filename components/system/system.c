#include <system.h>

#include "ld_offsets.h"
#include "heap_caps.h"
#include "memory_map.h"

#include <esp_clk.h>
#include <esp_idf_version.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <spi_flash.h>

size_t system_get_total_heap_size()
{
  return heap_caps_get_total_size(MALLOC_CAP_32BIT);
}

void system_image_info_get(struct system_image_info *info)
{
  info->iram_start = &_iram_start;
  info->iram_end = &_iram_end;
  info->iram_usage = &_iram_end - &_iram_start;
  info->iram_size = iram0_size;

  info->dram_start = &_data_start;
  info->dram_end = &_bss_end;
  info->dram_usage = &_bss_end - &_data_start;
  info->dram_size = dram0_size;

  info->flash_start = &_text_start;
  info->flash_end = &_rodata_end;
  info->flash_usage = &_rodata_end - &_text_start;

  for (unsigned i = 0; i < g_heap_region_num; i++) {
    heap_region_t *region = &g_heap_region[i];

    if (region->caps & MALLOC_CAP_EXEC) {
      info->iram_heap_start = region->start_addr;
      info->iram_heap_size = region->total_size;
    } else {
      info->dram_heap_start = region->start_addr;
      info->dram_heap_size = region->total_size;
    }
  }
}

void system_info_get(struct system_info *info)
{
  esp_chip_info(&info->esp_chip_info);
  system_image_info_get(&info->image_info);

  info->esp_app_desc = *esp_ota_get_app_description();

  info->esp_idf_version = esp_get_idf_version();
  info->spi_flash_chip_size = spi_flash_get_chip_size();
}

void system_status_get(struct system_status *status)
{
  status->reset_reason = esp_reset_reason();
  status->uptime = esp_timer_get_time();
  status->cpu_frequency = esp_clk_cpu_freq();
  status->total_heap_size = system_get_total_heap_size();
  status->free_heap_size = esp_get_free_heap_size();
  status->minimum_free_heap_size = esp_get_minimum_free_heap_size();
}
