#pragma once

#include <leds.h>
#include "leds.h"

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

struct leds_config;

struct leds_artnet_state;

struct leds_state {
  int index;
  const struct leds_config *config;

  struct leds *leds;

  xTaskHandle task;
  EventGroupHandle_t event_group;

  struct leds_test_state *test;
  struct leds_artnet_state *artnet;
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

/*
 * Variant of update_leds() that turns off all LEDs, and does NOT flash the status leds.
 */
int clear_leds(struct leds_state *state);

int test_leds_mode(struct leds_state *state, enum leds_test_mode mode);
int test_leds(struct leds_state *state);
