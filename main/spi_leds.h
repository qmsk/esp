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

enum spi_leds_artnet_mode {
  SPI_LEDS_RGB,
  SPI_LEDS_BGR,
  SPI_LEDS_GRB,
};

struct spi_leds_config {
  bool enabled;
  int protocol;
  int spi_clock;
  uint16_t delay;
  uint16_t count;

  int gpio_mode;
  uint16_t gpio_pin;

  bool test_enabled;

  bool artnet_enabled;
  uint16_t artnet_universe;
  int artnet_mode;
};

struct spi_leds_state {
  struct spi_leds *spi_leds;

  unsigned active;

  struct spi_leds_artnet {
    enum spi_leds_artnet_mode mode;
    struct artnet_dmx *dmx;

    xTaskHandle task;
    xQueueHandle queue;
  } artnet;
};

extern struct spi_leds_config spi_leds_configs[SPI_LEDS_COUNT];
extern struct spi_master *spi_master;
extern struct spi_leds_state spi_leds_states[SPI_LEDS_COUNT];

int update_spi_leds(struct spi_leds_state *state);
int test_spi_leds(struct spi_leds_state *state);

int init_spi_leds_artnet(struct spi_leds_state *state, unsigned index, const struct spi_leds_config *config);
