#pragma once

#include "user.h"

#include <cmd.h>

int init_user_leds();
int start_user_leds();

/* Update User LED state to reflect event */
void set_user_leds_state(enum user_state state);
void set_user_leds_activity(enum user_activity activity);
void set_user_leds_alert(enum user_alert alert);

extern const struct cmdtab user_leds_cmdtab;
