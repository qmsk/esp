#ifndef __USER_LED_H__
#define __USER_LED_H__

/* Status LED */

enum led_state {
    LED_OFF,
    LED_BLINK_SLOW,
    LED_BLINK_FAST,
    LED_ON,
};

int init_led(enum led_state state);

int led_set(enum led_state state);

#endif
