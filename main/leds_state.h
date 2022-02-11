#pragma once

#include "leds_artnet.h"

#include <leds.h>

struct leds_config;

struct leds_state {
  int index;
  const struct leds_config *config;

  struct leds *leds;

  unsigned active;

  struct leds_artnet artnet;
};

extern struct leds_state leds_states[LEDS_COUNT];

#if CONFIG_LEDS_SPI_ENABLED
# if !CONFIG_IDF_TARGET_ESP8266
  extern spi_host_device_t leds_spi_host;
# endif

  int init_leds_spi();
#endif

#if CONFIG_LEDS_UART_ENABLED
  extern struct uart *leds_uart;

  int init_leds_uart();
#endif

int update_leds(struct leds_state *state);

int test_leds_mode(struct leds_state *state, enum leds_test_mode mode);
int test_leds(struct leds_state *state);
