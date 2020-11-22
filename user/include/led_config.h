#ifndef __USER_LED_CONFIG_H__
#define __USER_LED_CONFIG_H__

#include <drivers/gpio.h>

#define LED_GPIO      GPIO_16 // integrated LED on NodeMCU
#define LED_INVERTED  1 // active-low

#define LED_TIMER_PERIOD      100   // ms
#define LED_BLINK_SLOW_CYCLE  18     // 1800ms off / 100ms on

#endif