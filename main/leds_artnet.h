#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "leds_state.h"
#include "leds_config.h"

#include <artnet.h>

struct leds_artnet_state {
  unsigned universe_count;
  unsigned universe_leds_count;

  struct artnet_dmx dmx;
  struct artnet_output **outputs;

  unsigned sync_bits; // bitmask of outputs waiting for sync
  unsigned sync_missed; // bitmask of outputs with missed sync
  TickType_t sync_tick; // tick for soft sync

  TickType_t timeout_tick; // tick for forced reset
};

unsigned count_leds_artnet_outputs();

int init_leds_artnet(struct leds_state *state, int index, const struct leds_config *config);
int start_leds_artnet(struct leds_state *state, const struct leds_config *config);

void leds_artnet_timeout_reset(struct leds_state *state);

/* Return next tick for possible artnet timeout */
TickType_t leds_artnet_wait(struct leds_state *state);

/* Need update for artnet state? */
bool leds_artnet_active(struct leds_state *state, EventBits_t event_bits);

/* Update LEDs from artnet outputs */
int leds_artnet_update(struct leds_state *state, EventBits_t event_bits);
