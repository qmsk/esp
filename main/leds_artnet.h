#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <artnet.h>

struct leds_config;
struct leds_state;

struct leds_artnet {
  struct artnet_dmx *dmx;

  unsigned universe_count;
  xTaskHandle task;
  struct artnet_output **outputs;
};

int init_leds_artnet(struct leds_state *state, int index, const struct leds_config *config);
int start_leds_artnet(struct leds_state *state, const struct leds_config *config);
