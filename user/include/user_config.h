#ifndef __USER__CONFIG_H__
#define __USER__CONFIG_H__

#include <c_types.h>
#include <spi_flash.h>

#include <lib/config.h>

#define SPIFFS_FLASH_SECTORS (1 * 1024 * 1024 / SPI_FLASH_SEC_SIZE) // 1MB at end of flash
#define SPIFFS_BLOCK_SIZE (SPI_FLASH_SEC_SIZE)
#define SPIFFS_PAGE_SIZE (SPI_FLASH_SEC_SIZE / 8)

#define USER_CONFIG_UART_BAUD_RATE 74880

extern struct config user_config;

int init_config(struct config *config);

#endif
