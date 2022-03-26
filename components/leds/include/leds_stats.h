#pragma once

#include <stats.h>

#include <sdkconfig.h>

struct leds_interface_stats {
#if CONFIG_LEDS_SPI_ENABLED
  struct leds_interface_spi_stats {
    struct stats_timer open;
    struct stats_timer tx;
  } spi;
#endif
#if CONFIG_LEDS_UART_ENABLED
  struct leds_interface_uart_stats {
    struct stats_timer open;
    struct stats_timer tx;
  } uart;
#endif
#if CONFIG_LEDS_I2S_ENABLED
  struct leds_interface_i2s_stats {
    struct stats_timer open;
    struct stats_timer tx;
  } i2s;
#endif
};

/*
 * Reset global per-interface stats.
 */
void leds_reset_interface_stats();

/*
 * Get a copy of the global per-interface stats.
 */
void leds_get_interface_stats(struct leds_interface_stats *stats);
