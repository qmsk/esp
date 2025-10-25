#include <leds_stats.h>
#include "leds.h"
#include "leds_stats.h"

#include <logging.h>

struct leds_interface_stats leds_interface_stats;

void leds_reset_interface_stats()
{
#if CONFIG_LEDS_SPI_ENABLED
  stats_timer_init(&leds_interface_stats.spi.open);
  stats_timer_init(&leds_interface_stats.spi.tx);
#endif
#if CONFIG_LEDS_UART_ENABLED
  stats_timer_init(&leds_interface_stats.uart.open);
  stats_timer_init(&leds_interface_stats.uart.tx);
#endif
#if LEDS_I2S_INTERFACE_COUNT > 0
  stats_timer_init(&leds_interface_stats.i2s0.open);
  stats_timer_init(&leds_interface_stats.i2s0.write);
  stats_timer_init(&leds_interface_stats.i2s0.flush);
#endif
#if LEDS_I2S_INTERFACE_COUNT > 1
  stats_timer_init(&leds_interface_stats.i2s1.open);
  stats_timer_init(&leds_interface_stats.i2s1.write);
  stats_timer_init(&leds_interface_stats.i2s1.flush);
#endif
}

void leds_get_interface_stats(struct leds_interface_stats *stats)
{
  *stats = leds_interface_stats;
}

struct leds_interface_i2s_stats *leds_interface_i2s_stats(enum leds_interface interface)
{
  switch(interface) {
  #if LEDS_I2S_INTERFACE_COUNT > 0
    case LEDS_INTERFACE_I2S0:
      return &leds_interface_stats.i2s0;
  #endif
  #if LEDS_I2S_INTERFACE_COUNT > 1
    case LEDS_INTERFACE_I2S1:
      return &leds_interface_stats.i2s1;
  #endif
    default:
      LOG_FATAL("%u", interface);
  }
}
