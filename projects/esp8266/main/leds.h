#pragma once

#include "leds_config.h"
#include "leds_init.h"
#include "leds_cmd.h"

#include <leds.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define LEDS_GPIO_OFF (-1)
#define LEDS_SPI_CLOCK (SPI_CLOCK_1MHZ)

#define LEDS_ARTNET_UNIVERSE_LEDS (512 / 3)

struct leds_config {
  bool enabled;

  int interface;
  int protocol;
  uint16_t count;

  int spi_clock;
  uint16_t spi_delay;

  int gpio_mode;
  uint16_t gpio_pin;

  bool test_enabled;

  bool artnet_enabled;
  uint16_t artnet_universe_start;
  uint16_t artnet_universe_count;
  uint16_t artnet_universe_step;
  uint16_t artnet_dmx_addr;
  uint16_t artnet_dmx_leds;
  int artnet_leds_format;
  uint16_t artnet_leds_segment;
};

struct leds_state {
  const struct leds_config *config;

  struct leds *leds;

  unsigned active;

  struct leds_artnet {
    struct artnet_dmx *dmx;

    unsigned universe_count;
    xTaskHandle task;
    xQueueHandle *queues;
  } artnet;
};

extern struct leds_config leds_configs[LEDS_COUNT];
extern struct spi_master *spi_master;
extern struct leds_state leds_states[LEDS_COUNT];

int update_leds(struct leds_state *state);
int test_leds(struct leds_state *state, enum leds_test_mode mode);

int init_leds_artnet(struct leds_state *state, unsigned index, const struct leds_config *config);
