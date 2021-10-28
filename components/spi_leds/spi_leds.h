#pragma once

#include <spi_leds.h>
#include "apa102.h"
#include "p9813.h"
#include "ws2812b.h"

#include <stdbool.h>


struct spi_leds {
  struct spi_leds_options options;

  // protocol state
  union {
    struct spi_leds_protocol_apa102 apa102;
    struct spi_leds_protocol_p9813 p9813;
    struct spi_leds_protocol_ws2812b ws2812b;
  } state;

  // if false, all leds are inactive
  bool active;
};

int spi_leds_init(struct spi_leds *spi_leds, const struct spi_leds_options *options);

/* spi.c */
int spi_leds_tx_spi(const struct spi_leds_options *options, enum spi_mode spi_mode, void *buf, size_t size);
