#include <leds_stats.h>
#include "leds_stats.h"

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
#if CONFIG_LEDS_I2S_ENABLED
  stats_timer_init(&leds_interface_stats.i2s.open);
  stats_timer_init(&leds_interface_stats.i2s.write);
  stats_timer_init(&leds_interface_stats.i2s.flush);
#endif
}

void leds_get_interface_stats(struct leds_interface_stats *stats)
{
  *stats = leds_interface_stats;
}
