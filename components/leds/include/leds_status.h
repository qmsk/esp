#pragma once

#include <leds.h>

struct leds_limit_status {
  unsigned count; /* LEDs count */
  unsigned limit; /* Configured limit */
  unsigned power; /* Total LED power set */
  unsigned output; /* Limited LED power output */
};

/*
 * Get power limit configured.
 *
 * @return 0.0 .. 1.0
 *         0.0 -> maximum power limit
 *         1.0 -> no power limit
 */
static inline float leds_limit_status_configured(const struct leds_limit_status *status)
{
  if (status->limit && status->count) {
    return status->limit / status->count;
  } else {
    return 1.0f;
  }
}

/*
 * Get power limit utilization.
 *
 * @return 0.0 .. 1.0 .. +inf
 *         0.0 -> all LEDs are off, or no limit configured
 *         1.0 -> at power limit
 *        >1.0 -> power limiting active
 */
static inline float leds_limit_status_utilization(const struct leds_limit_status *status)
{
  if (status->limit) {
    return status->power / status->limit;
  } else {
    return 0.0f;
  }
}

/*
 * Get power limit applied.
 *
 * This is applied using integer math internally, the floating point value is an approximation.
 *
 * @return 0.0 .. 1.0, with 1.0 meaning power is not being limited and numbers closer to 0.0 meaning more power limiting is being applied.
 */
static inline float leds_limit_status_active(const struct leds_limit_status *status)
{
  if (status->power) {
    return status->output / status->power;
  } else {
    return 1.0f;
  }
}

void leds_get_limit_total_status(struct leds *leds, struct leds_limit_status *total_status);
void leds_get_limit_groups_status(struct leds *leds, struct leds_limit_status *group_status, size_t *size);
