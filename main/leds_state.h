#pragma once

#include <leds.h>
#include "leds.h"
#include "leds_status.h"
#include "user.h"

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

struct leds_config;

struct leds_test_state;
struct leds_artnet_state;
struct leds_sequence_state;

enum leds_update_state {
  LEDS_UPDATE_NONE,
  LEDS_UPDATE_STATIC,
  LEDS_UPDATE_TEST,
  LEDS_UPDATE_SEQUENCE,
  LEDS_UPDATE_ARTNET,
  LEDS_UPDATE_CMD,
  LEDS_UPDATE_HTTP,
};

struct leds_state {
  int index;
  const struct leds_config *config;
  SemaphoreHandle_t mutex;
  
  struct leds *leds;
  TickType_t update_tick;
  enum leds_update_state update_state;

  xTaskHandle task;
  EventGroupHandle_t event_group;

  struct leds_test_state *test;
  struct leds_artnet_state *artnet;
  struct leds_sequence_state *sequence;
  struct leds_static_state {
    struct leds_color color;
  } static_;

  struct leds_status_timers status_timers;
  struct leds_status_timer_metrics status_timer_metrics;
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
  int init_leds_i2s(unsigned port);

# if CONFIG_IDF_TARGET_ESP8266
  int check_leds_i2s(struct leds_state *state);
# endif
#endif

/* 
 * Optional persistent leds interface setup.
 */
int setup_leds(struct leds_state *state);

/* 
 * Reset leds interface setup if necessary.
 */
int reset_leds(struct leds_state *state);

/*
 * Update LEDs output.
 */
int output_leds(struct leds_state *state);

/* Lock LEDs for out-of-task update */
int start_leds_update(struct leds_state *state, enum leds_update_state update_state);

/* Release LEDS and trigger task */
void end_leds_update(struct leds_state *state);
