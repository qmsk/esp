#pragma once

#include <leds.h>
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
    struct stats_timer write;
    struct stats_timer flush;
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

/**  */
struct leds_limit_stats {
  unsigned limit; /* Configured limit */
  unsigned power; /* Total LED power set */
  unsigned output; /* Limited LED power output */
};
/*
 * Get power limit utilization from last leds_tx() operation.
 * *
 * @return 0.0 .. 1.0 .. +inf
 *         0.0 -> all LEDs are off, or no limit configured
 *         1.0 -> at power limit
 *        >1.0 -> power limiting active
 */
static inline float leds_limit_stats_utilization(const struct leds_limit_stats *stats)
{
  if (stats->limit) {
    return stats->power / stats->limit;
  } else {
    return 0.0f;
  }
}

/*
 * Get power limit applied to last leds_tx() operation.
 *
 * This is applied using integer math internally, the floating point value is an approximation.
 *
 * @return 0.0 .. 1.0, with 1.0 meaning power is not being limited and numbers closer to 0.0 meaning more power limiting is being applied.
 */
static inline float leds_limit_stats_active(const struct leds_limit_stats *stats)
{
  if (stats->power) {
    return stats->output / stats->power;
  } else {
    return 1.0f;
  }
}

void leds_get_limit_total_stats(struct leds *leds, struct leds_limit_stats *total_stats);
void leds_get_limit_group_stats(struct leds *leds, struct leds_limit_stats *group_stats, size_t *size);
