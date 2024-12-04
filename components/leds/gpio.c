#include "gpio.h"

#if CONFIG_LEDS_GPIO_ENABLED
    void leds_gpio_setup (const struct leds_interface_options_gpio *options)
    {
        if (!options->gpio_options) {
            return;
        }

        switch (options->mode) {
            case LEDS_GPIO_MODE_NONE:
                break;
            
            case LEDS_GPIO_MODE_SETUP:
            case LEDS_GPIO_MODE_ACTIVE:
                gpio_out_set(options->gpio_options, options->pins);
                break;
        }
    }

    void leds_gpio_close (const struct leds_interface_options_gpio *options)
    {
        if (!options->gpio_options) {
            return;
        }

        switch (options->mode) {
            case LEDS_GPIO_MODE_NONE:
                break;
            
            case LEDS_GPIO_MODE_SETUP:
                break;

            case LEDS_GPIO_MODE_ACTIVE:
                gpio_out_clear(options->gpio_options);
                break;
        }

    }
#endif
