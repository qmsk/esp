#pragma once

#include <esp8266/spi_struct.h>

/* ESP8266 only has one output device */
#define SPI_DEV (SPI1)

/* Undocumented */
#define REG_SPI_INT_STATUS 0x3ff00020
#define SPI_INT_STATUS_SPI0 BIT4
#define SPI_INT_STATUS_SPI1 BIT7
