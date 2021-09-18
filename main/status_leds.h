#pragma once

#include "user_event.h"

#include <cmd.h>

int init_status_leds();

/* Update User LED state to reflect event */
void status_led_event(enum user_event event);

extern const struct cmdtab status_leds_cmdtab;
