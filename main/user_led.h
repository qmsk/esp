#pragma once

#include "user_event.h"

#include <cmd.h>

int init_user_led();

/* Update User LED state to reflect event */
void user_led_event(enum user_event event);

extern const struct cmdtab user_led_cmdtab;
