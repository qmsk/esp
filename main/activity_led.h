#pragma once

#include <config.h>

extern const struct configtab activity_led_configtab[];

int init_activity_led();

/* Update Activity LED state to reflect event */
void activity_led_event();
