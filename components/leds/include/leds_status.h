#pragma once

#include <leds.h>

struct leds_limit_status {
  unsigned count; /* LEDs count */
  unsigned limit; /* Configured limit */
  unsigned power; /* Total LED power set */
  unsigned output; /* Limited LED power output */
};

/*
 * Get power usage before configured limits.
 *
 * @return 0.0 .. 1.0
 *         0.0 -> all off
 *         1.0 -> maximum power
 */
static inline float leds_limit_status_power(const struct leds_limit_status *status)
{
  if (status->power && status->count) {
    return (float) status->power / (float) status->count;
  } else {
    return 0.0f;
  }
}

/*
 * Get power limit configured.
 *
 * @return 0.0 .. 1.0
 *         0.0 -> maximum power limit
 *         1.0 -> no power limit
 */
static inline float leds_limit_status_limit(const struct leds_limit_status *status)
{
  if (status->limit && status->count) {
    return (float) status->limit / (float) status->count;
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
static inline float leds_limit_status_util(const struct leds_limit_status *status)
{
  if (status->limit) {
    return (float) status->power / (float) status->limit;
  } else {
    return 0.0f;
  }
}

/*
 * Get applied output power limit.
 *
 * This is applied using integer math internally, the floating point value is an approximation.
 *
 * @return 0.0 .. 1.0, with 1.0 meaning power is not being limited and numbers closer to 0.0 meaning more power limiting is being applied.
 */
static inline float leds_limit_status_applied(const struct leds_limit_status *status)
{
  if (status->power) {
    return (float) status->output / (float) status->power;
  } else {
    return 1.0f;
  }

}
/*
 * Get output power used.
 *
 * This is applied using integer math internally, the floating point value is an approximation.
 *
 * @return 0.0 .. 1.0
 *         0.0 -> all LEDs are off, minimum output power
 *         1.0 -> at power limit
 */
static inline float leds_limit_status_output(const struct leds_limit_status *status)
{
  if (status->limit) {
    return (float) status->output / (float) status->limit;
  } else {
    return 0.0f;
  }
}

void leds_get_limit_total_status(struct leds *leds, struct leds_limit_status *total_status);
void leds_get_limit_groups_status(struct leds *leds, struct leds_limit_status *group_status, size_t *size);
