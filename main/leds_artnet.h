#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <artnet.h>

struct leds_config;
struct leds_state;

struct leds_artnet {
  unsigned universe_count;

  struct artnet_dmx *dmx;
  struct artnet_output **outputs;

  xTaskHandle task;
  EventGroupHandle_t event_group;
};

int init_leds_artnet(struct leds_state *state, int index, const struct leds_config *config);
int start_leds_artnet(struct leds_state *state, const struct leds_config *config);
