#ifndef __DRIVERS_SPI_H__
#define __DRIVERS_SPI_H__

#include "spi_config.h"
#include <stdint.h>

enum SPI {
  SPI_1   = 1,
};

void SPI_SetupMaster(enum SPI spi, struct SPI_MasterConfig config);

typedef uint32_t SPI_DataBuf[16];

struct SPI_Operation {
  unsigned command_bits, address_bits, dummy_cycles, data_bits;

  uint16_t command;
  uint32_t address;
  SPI_DataBuf data_buf;
};

void SPI_Send(enum SPI spi, const struct SPI_Operation *op);

#endif
