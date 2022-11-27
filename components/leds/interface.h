#pragma once

#include <leds.h>

#if CONFIG_LEDS_I2S_ENABLED
# include "interfaces/i2s.h"
#endif
#if CONFIG_LEDS_SPI_ENABLED
# include "interfaces/spi.h"
#endif
#if CONFIG_LEDS_UART_ENABLED
# include "interfaces/uart.h"
#endif

union leds_interface_state {
#if CONFIG_LEDS_SPI_ENABLED
  struct leds_interface_spi spi;
#endif
#if CONFIG_LEDS_I2S_ENABLED
  struct leds_interface_i2s i2s;
#endif
#if CONFIG_LEDS_UART_ENABLED
  struct leds_interface_uart uart;
#endif
};
