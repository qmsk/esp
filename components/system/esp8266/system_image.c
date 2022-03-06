#include <system.h>

#include <esp_spi_flash.h>

#include "ld_offsets.h"
#include "heap_caps.h"
#include "memory_map.h"

#define PTR(sym) (&sym)
#define ADDR(sym) ((unsigned)(&sym))
#define SIZE(start, end) (ADDR(end) - ADDR(start))

void system_image_info_get(struct system_image_info *info)
{
  info->irom_start = ADDR(_text_start);
  info->irom_end = ADDR(_rodata_end);
  info->irom_size = SIZE(_text_start, _rodata_end);

  info->iram_total_size = iram0_size;
  info->iram_start = ADDR(_iram_start);
  info->iram_end = ADDR(_iram_end);
  info->iram_size = SIZE(_iram_start, _iram_end);

  info->dram_total_size = dram0_size;
  info->dram_start = ADDR(_data_start);
  info->dram_end = ADDR(_bss_end);
  info->dram_size = SIZE(_data_start, _bss_end);

  // no drom
  info->drom_start = info->drom_end = 0;
  info->drom_size = 0;

  for (unsigned i = 0; i < g_heap_region_num; i++) {
    heap_region_t *region = &g_heap_region[i];

    if (region->caps & MALLOC_CAP_EXEC) {
      info->iram_heap_start = (unsigned) region->start_addr;
      info->iram_heap_end = (unsigned) region->start_addr + region->total_size;
      info->iram_heap_size = region->total_size;
    } else {
      info->dram_heap_start = (unsigned) region->start_addr;
      info->dram_heap_end = (unsigned) region->start_addr + region->total_size;
      info->dram_heap_size = region->total_size;
    }
  }

  info->flash_size = spi_flash_get_chip_size();
  info->flash_usage = info->irom_size + info->drom_size;
}
