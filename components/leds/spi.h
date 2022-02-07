#pragma once

#include <leds.h>

#if CONFIG_LEDS_SPI_ENABLED
  int leds_tx_spi(const struct leds_options *options, enum spi_mode spi_mode, void *buf, size_t size);
#endif
