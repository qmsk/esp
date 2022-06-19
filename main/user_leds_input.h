#pragma once

#include <user_leds.h>

/**
 * Read current input state.
 *
 * Returns < 0 on error, 0 if idle, 1 if pressed.
 */
int get_user_led(enum user_led led);

/**
 * Read input events from queue.
 *
 * Return 0 on sucess, 1 on timeout.
 */
int read_user_leds_input(struct user_leds_input *inputp, TickType_t timeout);
