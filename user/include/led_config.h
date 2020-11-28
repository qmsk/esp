#ifndef __USER_LED_CONFIG_H__
#define __USER_LED_CONFIG_H__

#include <drivers/gpio.h>

#define LED_GPIO      GPIO_16 // integrated LED on NodeMCU
#define LED_INVERTED  1 // active-low

// 100ms on / 1800ms off
#define LED_BLINK_SLOW_PERIOD_ON 100
#define LED_BLINK_SLOW_PERIOD_OFF 1800

// 100ms on / 100ms off
#define LED_BLINK_FAST_PERIOD 100

// 100ms on
#define LED_BLINK_PERIOD 100

#endif
