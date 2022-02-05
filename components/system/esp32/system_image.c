#include <system.h>

#include <logging.h>

#include <esp_heap_caps.h>
#include <esp_spi_flash.h>

#include "ld_offsets.h"

#define PTR(sym) (&sym)
#define ADDR(sym) ((unsigned)(&sym))
#define SIZE(start, end) (ADDR(end) - ADDR(start))

void system_image_info_get(struct system_image_info *info)
{
  LOG_DEBUG("dram0        start=%#08x end=%#08x size=%#08x", ADDR(_data_start), ADDR(_bss_end), SIZE(_data_start, _bss_end));
  LOG_DEBUG("dram0 data   start=%#08x end=%#08x size=%#08x", ADDR(_data_start), ADDR(_data_end), SIZE(_data_start, _data_end));
  LOG_DEBUG("dram0 noinit start=%#08x end=%#08x size=%#08x", ADDR(_noinit_start), ADDR(_noinit_end), SIZE(_noinit_start, _noinit_end));
  LOG_DEBUG("dram0 bss    start=%#08x end=%#08x size=%#08x", ADDR(_bss_start), ADDR(_bss_end), SIZE(_bss_start, _bss_end));
  LOG_DEBUG("dram0 heap   start=%#08x end=%#08x size=%#08x", ADDR(_heap_start), ADDR(_heap_end), SIZE(_heap_start, _heap_end));

  LOG_DEBUG("flash rodata start=%#08x end=%#08x size=%#08x", ADDR(_rodata_start), ADDR(_rodata_end), SIZE(_rodata_start, _rodata_end));
  LOG_DEBUG("flash text   start=%#08x end=%#08x size=%#08x", ADDR(_text_start), ADDR(_text_end), SIZE(_text_start, _text_end));

  LOG_DEBUG("iram0        start=%#08x end=%#08x size=%#08x", ADDR(_iram_start), ADDR(_iram_end), SIZE(_iram_start, _iram_end));
  LOG_DEBUG("iram0 text   start=%#08x end=%#08x size=%#08x", ADDR(_iram_text_start), ADDR(_iram_text_end), SIZE(_iram_text_start, _iram_text_end));
  LOG_DEBUG("iram0 data   start=%#08x end=%#08x size=%#08x", ADDR(_iram_data_start), ADDR(_iram_data_start), SIZE(_iram_data_start, _iram_data_end));
  LOG_DEBUG("iram0 bss    start=%#08x end=%#08x size=%#08x", ADDR(_iram_bss_start), ADDR(_iram_bss_end), SIZE(_iram_bss_start, _iram_bss_end));

  info->iram_total_size = iram0_size;
  info->dram_total_size = dram0_size;

  info->irom_start = ADDR(_text_start);
  info->irom_end = ADDR(_text_end);
  info->irom_size = SIZE(_text_start, _text_end);

  info->iram_start = ADDR(_iram_start);
  info->iram_end = ADDR(_iram_end);
  info->iram_size = SIZE(_iram_start, _iram_end);
  info->iram_heap_start = ADDR(_iram_end);
  info->iram_heap_end = iram0_org + iram0_size;
  info->iram_heap_size = iram0_org + iram0_size - ADDR(_iram_end);

  info->dram_start = ADDR(_data_start);
  info->dram_end = ADDR(_bss_end);
  info->dram_size = SIZE(_data_start, _bss_end);
  info->dram_heap_start = ADDR(_heap_start);
  info->dram_heap_end = dram0_org + dram0_size;
  info->dram_heap_size = dram0_org + dram0_size - ADDR(_heap_start);

  info->drom_start = ADDR(_rodata_start);
  info->drom_end = ADDR(_rodata_end);
  info->drom_size = SIZE(_rodata_start, _rodata_end);

  info->flash_size = spi_flash_get_chip_size();
  info->flash_usage = info->irom_size + info->drom_size;
}
