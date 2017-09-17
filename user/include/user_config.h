#ifndef __USER__CONFIG_H__
#define __USER__CONFIG_H__

#include <c_types.h>
#include <spi_flash.h>

#define SPIFFS_FLASH_SECTORS (1 * 1024 * 1024 / SPI_FLASH_SEC_SIZE) // 1MB at end of flash
#define SPIFFS_BLOCK_SIZE (SPI_FLASH_SEC_SIZE)
#define SPIFFS_PAGE_SIZE (SPI_FLASH_SEC_SIZE / 8)

#define USER_CONFIG_UART_BAUD_RATE 74880

#include "local/user_config.h"
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

#ifndef USER_CONFIG_WIFI_SSID
  #error "include/local/user_config.h: #define USER_CONFIG_WIFI_SSID"
#endif
#ifndef USER_CONFIG_WIFI_PASSWORD
  #error "include/local/user_config.h: #define USER_CONFIG_WIFI_PASSWORD"
#endif

#endif
