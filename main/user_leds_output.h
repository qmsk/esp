#pragma once

#include "user_leds_config.h"
#include <user_leds.h>

#include <sdkconfig.h>

int override_user_led(enum user_led led, enum user_leds_state state);
int revert_user_led(enum user_led led);
int set_user_led(enum user_led led, enum user_leds_state state);
