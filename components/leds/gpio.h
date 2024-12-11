#pragma once

#include <leds.h>

#if CONFIG_LEDS_GPIO_ENABLED
    void leds_gpio_setup (const struct leds_interface_options_gpio *options);
    void leds_gpio_close (const struct leds_interface_options_gpio *options);
#endif
