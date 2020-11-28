#ifndef __USER_LED_H__
#define __USER_LED_H__

/* Status LED */

enum led_mode {
    LED_OFF,
    LED_ON,
    LED_SLOW,   // blink slow
    LED_FAST,   // blink fast
    LED_BLINK,  // blink once
};

int init_led(enum led_mode mode);

int led_set(enum led_mode mode);

#endif
