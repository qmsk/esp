#pragma once

#include "leds_config.h"
#include "leds_state.h"

#include <artnet.h>

#define LEDS_EVENT_BITS         0x003f

enum leds_event_bit {
    LEDS_EVENT_TEST_BIT         = 0,
    LEDS_EVENT_ARTNET_DMX_BIT   = 1,
    LEDS_EVENT_ARTNET_SYNC_BIT  = 2,
    LEDS_EVENT_SEQUENCE_BIT     = 3,
    LEDS_EVENT_STATIC_BIT       = 4,
    LEDS_EVENT_UPDATE_BIT       = 5,
};

int init_leds_task(struct leds_state *state, const struct leds_config *config);
int start_leds_task(struct leds_state *state, const struct leds_config *config);

void notify_leds_task(struct leds_state *state, EventBits_t bits);
void notify_leds_tasks(EventBits_t bits);

/* Trigger task for leds output update */
int update_leds(struct leds_state *state, enum user_activity activity);
