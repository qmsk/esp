#pragma once

#include "user.h"

#include <cmd.h>

int init_status_leds();
int start_status_leds();

/* Update User LED state to reflect event */
void status_leds_state(enum user_state state);
void status_leds_activity(enum user_activity activity);
void status_leds_alert(enum user_alert alert);

extern const struct cmdtab status_leds_cmdtab;
