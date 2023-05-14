#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "leds_state.h"

#include <fseq.h>

struct leds_sequence_state {
  // XXX: wasteful to have 1+N*2 copies of each fseq_frame
  xQueueHandle queue; // struct fseq_frame

  struct fseq_frame *fseq_frame;
};

int init_leds_sequence();
int config_leds_sequence(struct leds_state *state, const struct leds_config *config);
int start_leds_sequence();

/* Return next tick for sequence frame */
TickType_t leds_sequence_wait(struct leds_state *state);

/* Need update for sequencestate? */
bool leds_sequence_active(struct leds_state *state, EventBits_t event_bits);

/* Update LEDs from sequence */
int leds_sequence_update(struct leds_state *state, EventBits_t event_bits);
