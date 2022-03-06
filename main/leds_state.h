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

#if CONFIG_LEDS_GPIO_ENABLED
  int init_leds_gpio();
#endif

#if CONFIG_LEDS_SPI_ENABLED
  int init_leds_spi();
#endif

#if CONFIG_LEDS_UART_ENABLED
  int init_leds_uart();
#endif

#if CONFIG_LEDS_I2S_ENABLED
  int init_leds_i2s();
  int check_leds_i2s(struct leds_state *state);
#endif

int update_leds(struct leds_state *state);

int test_leds_mode(struct leds_state *state, enum leds_test_mode mode);
int test_leds(struct leds_state *state);
