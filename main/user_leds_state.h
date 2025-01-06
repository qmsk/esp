#pragma once

#include <user_leds.h>
#include "user_leds_config.h"

extern const struct config_enum user_leds_state_enum[];

extern enum user_leds_state user_leds_state[USER_LEDS_COUNT];
