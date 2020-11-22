#ifndef __USER__CONFIG_H__
#define __USER__CONFIG_H__

#include <c_types.h>
#include <spi_flash.h>
#include "config.h"

#define SPIFFS_FLASH_SECTORS (1 * 1024 * 1024 / SPI_FLASH_SEC_SIZE) // 1MB at end of flash
#define SPIFFS_BLOCK_SIZE (SPI_FLASH_SEC_SIZE)
#define SPIFFS_PAGE_SIZE (SPI_FLASH_SEC_SIZE / 8)

#define USER_CONFIG_UART_BAUD_RATE 74880
#define USER_CONFIG_WIFI_SSID ""
#define USER_CONFIG_WIFI_PASSWORD ""

#include "artnet_config.h"
#include "dmx_config.h"
#include "p9813_config.h"

struct user_config {
  uint16 version;
  char wifi_ssid[32];
  char wifi_password[64];

  struct artnet_config artnet;
  struct dmx_config dmx;
  struct p9813_config p9813;
};

#define USER_CONFIG_VERSION 4

extern struct user_config user_config;
extern const struct config_tab user_configtab[];

int init_config(struct user_config *config);

#endif
