#ifndef __USER__CONFIG_H__
#define __USER__CONFIG_H__

#include <c_types.h>
#include <spi_flash.h>

#include <lib/config.h>

#define SPIFFS_FLASH_SECTORS (1 * 1024 * 1024 / SPI_FLASH_SEC_SIZE) // 1MB at end of flash
#define SPIFFS_BLOCK_SIZE (SPI_FLASH_SEC_SIZE)
#define SPIFFS_PAGE_SIZE (SPI_FLASH_SEC_SIZE / 8)

#define USER_CONFIG_UART_BAUD_RATE 74880

#include "wifi_config.h"
#include "artnet_config.h"
#include "dmx_config.h"
#include "p9813_config.h"

struct user_config {
  uint16 version;

  struct wifi_config wifi;
  struct artnet_config artnet;
  struct dmx_config dmx;
  struct p9813_config p9813;
};

#define USER_CONFIG_VERSION 4

extern struct user_config user_config;
extern struct config user_configmeta;

int init_config(struct config *config);

#endif
