#pragma once

#include "spi_leds_config.h"
#include "spi_leds_init.h"
#include "spi_leds_cmd.h"

#include <spi_leds.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define SPI_LEDS_GPIO_OFF (-1)
#define SPI_LEDS_CLOCK (SPI_CLOCK_1MHZ)

#define SPI_LEDS_ARTNET_UNIVERSE_LEDS (512 / 3)

struct spi_leds_config {
  bool enabled;

  int protocol;
  uint16_t count;

  int spi_clock;
  uint16_t spi_delay;

  int gpio_mode;
  uint16_t gpio_pin;

  bool test_enabled;

  bool artnet_enabled;
  int artnet_mode;
  uint16_t artnet_universe_start;
  uint16_t artnet_universe_count;
  uint16_t artnet_universe_step;
  uint16_t artnet_universe_leds;
  uint16_t artnet_dmx_addr;
  uint16_t artnet_leds_segment;
};

struct spi_leds_state {
  const struct spi_leds_config *config;

  struct spi_leds *spi_leds;

  unsigned active;

  struct spi_leds_artnet {
    struct artnet_dmx *dmx;

    unsigned universe_count;
    xTaskHandle task;
    xQueueHandle *queues;
  } artnet;
};

extern struct spi_leds_config spi_leds_configs[SPI_LEDS_COUNT];
extern struct spi_master *spi_master;
extern struct spi_leds_state spi_leds_states[SPI_LEDS_COUNT];

int update_spi_leds(struct spi_leds_state *state);
int test_spi_leds(struct spi_leds_state *state);

int init_spi_leds_artnet(struct spi_leds_state *state, unsigned index, const struct spi_leds_config *config);
