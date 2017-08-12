#include <c_types.h>
#include <spi_flash.h>
#include <esp_spiffs.h>
#include "user_config.h"

static int config_spiffs(struct esp_spiffs_config *spiffs_config)
{
    uint32 rfcal_flash_sect = user_rf_cal_sector_set();
    uint32 spiffs_flash_sect = rfcal_flash_sect - SPIFFS_FLASH_SECTORS;

    spiffs_config->phys_size = SPIFFS_FLASH_SECTORS * SPI_FLASH_SEC_SIZE;
    spiffs_config->phys_addr = spiffs_flash_sect * SPI_FLASH_SEC_SIZE;
    spiffs_config->phys_erase_block = SPI_FLASH_SEC_SIZE;
    spiffs_config->log_block_size = SPIFFS_BLOCK_SIZE;
    spiffs_config->log_page_size = SPIFFS_PAGE_SIZE;
    spiffs_config->fd_buf_size = 32 * 8; // ~8 32-byte FDs
    spiffs_config->cache_buf_size = (SPIFFS_PAGE_SIZE + 32) * 8; // ~8 pages

    return 0;
}

int init_spiffs()
{
  struct esp_spiffs_config spiffs_config = {};
  int err;

  if (config_spiffs(&spiffs_config)) {
    printf("ERROR spiffs: config\n");
    return -1;
  }

  printf("INFO spiffs: size=%x flash=%x:%x\n",
    spiffs_config.phys_size,
    spiffs_config.phys_addr,
    spiffs_config.phys_addr + spiffs_config.phys_size - 1
  );

  if ((err = esp_spiffs_init(&spiffs_config))) {
    printf("ERROR spiffs: init %d\n", err);
    return -1;
  }
}
