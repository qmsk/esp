#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include <c_types.h>
#include <spi_flash.h>

#define SPIFFS_FLASH_SECTORS (1 * 1024 * 1024 / SPI_FLASH_SEC_SIZE) // 1MB at end of flash
#define SPIFFS_BLOCK_SIZE (SPI_FLASH_SEC_SIZE)
#define SPIFFS_PAGE_SIZE (SPI_FLASH_SEC_SIZE / 8)

#define USER_CONFIG_UART_BAUD_RATE 74880

struct user_config {
  char wifi_ssid[32];
  char wifi_password[64];
};

#include "local/user_config.h"

#ifndef USER_CONFIG_WIFI_SSID
  #error "include/local/user_config.h: #define USER_CONFIG_WIFI_SSID"
#endif
#ifndef USER_CONFIG_WIFI_PASSWORD
  #error "include/local/user_config.h: #define USER_CONFIG_WIFI_PASSWORD"
#endif

#endif
