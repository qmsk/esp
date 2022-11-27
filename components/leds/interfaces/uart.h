#pragma once

#include <leds.h>

#include "../limit.h"

#if CONFIG_LEDS_UART_ENABLED
  enum leds_interface_uart_mode {
    LEDS_INTERFACE_UART_MODE_NONE = 0,

    LEDS_INTERFACE_UART_MODE_24B3I7_0U4_80U,  // 24 bits @ 3 data bits / 7-bit inverted frame @ 0.4u symbols = 2.5M baud, 1.2us / data bit, 80uS reset
    LEDS_INTERFACE_UART_MODE_24B2I8_0U25_50U, // 24 bits @ 2 data bits / 8-bit inverted frame @ 0.25u symbols = 4M baud, 1.25us / data bit, 50uS reset
    LEDS_INTERFACE_UART_MODE_32B2I6_0U3_80U,  // 32 bits @ 2 data bits / 6-bit inverted frame @ 0.3u symbols = 3.33333333M baud, 1.2us / data bit, 80uS reset
  };

  union leds_interface_uart_func {
    void (*uart_mode_24B3I7)(uint16_t buf[4], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
    void (*uart_mode_24B2I8)(uint16_t buf[6], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
    void (*uart_mode_32B2I6)(uint16_t buf[8], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
  };

  union leds_interface_uart_buf {
    uint16_t uart_mode_24B3I7[4];
    uint16_t uart_mode_24B2I8[6];
    uint16_t uart_mode_32B2I6[8];
  };

  #define LEDS_INTERFACE_UART_FUNC(type, func) ((union leds_interface_uart_func) { .type = func })

  struct leds_interface_uart {
    enum leds_interface_uart_mode mode;
    union leds_interface_uart_func func;
    union leds_interface_uart_buf buf;

    struct uart *uart;
    struct uart_options uart_options;

    struct gpio_options *gpio_options;
    gpio_pins_t gpio_out_pins;
  };

  int leds_interface_uart_init(struct leds_interface_uart *interface, const struct leds_interface_uart_options *options, enum leds_interface_uart_mode mode, union leds_interface_uart_func func);
  int leds_interface_uart_tx(struct leds_interface_uart *interface, const struct leds_color *pixels, unsigned count, const struct leds_limit *limit);
#endif
