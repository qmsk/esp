#ifndef __USER_P9813_PROTOCOL_H__
#define __USER_P9813_PROTOCOL_H__

#include <drivers/spi.h>

static const enum SPI_Mode p9813_spi_mode = SPI_MODE_0;
static const enum SPI_Clock p9813_spi_clock = SPI_CLOCK_1MHZ;

struct __attribute__((packed)) p9813_packet {
  uint8_t control;
  uint8_t r, g, b;
};

#endif
