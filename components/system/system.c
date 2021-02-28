#include <system.h>

#include "ld_offsets.h"
#include "heap_caps.h"
#include "memory_map.h"

void system_image_info(struct system_image_info *info)
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

size_t system_get_total_heap_size()
{
  return heap_caps_get_total_size(MALLOC_CAP_32BIT);
}
