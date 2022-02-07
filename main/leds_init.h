#pragma once

#include "leds_state.h"
#include "leds_config.h"

int init_leds_artnet(struct leds_state *state, unsigned index, const struct leds_config *config);
