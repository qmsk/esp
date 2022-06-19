#pragma once

#include <user_leds.h>

/**
 *
 * Return 0 on sucess, 1 on timeout.
 */
int read_user_leds_input(struct user_leds_input *inputp, TickType_t timeout);
