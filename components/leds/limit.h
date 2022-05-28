#pragma once

#include <stdint.h>

// use integer math with a fixed power-of-two shift
// NOTE: `power` will have a maximum of 16 bits set, shifted value must not overflow on 32-bit operations
#define LEDS_LIMIT_TOTAL_SHIFT 8
#define LEDS_LIMIT_GROUP_SHIFT 8

/* Use integer math to scale LED channel values */
struct leds_limit {
  unsigned group_count;
  unsigned group_size; // number of LEDs per group, 0 for no group limit

  uint16_t total_multipler; // total limit
  uint16_t *group_multipliers; // [count] per group limit
};

int leds_limit_init(struct leds_limit *limit, unsigned groups, unsigned leds);

/* Returns limited power */
unsigned leds_limit_set_group(struct leds_limit *limit, unsigned group, unsigned group_limit, unsigned power);

/* Returns limited power */
unsigned leds_limit_set_total(struct leds_limit *limit, unsigned total_limit, unsigned power);

static inline uint8_t leds_limit_uint8(const struct leds_limit *limit, unsigned index, uint8_t value)
{
  uint16_t group_multiplier = limit->group_size ? limit->group_multipliers[index / limit->group_size] : (1 << LEDS_LIMIT_GROUP_SHIFT);

  return (value * group_multiplier * limit->total_multipler) >> (LEDS_LIMIT_GROUP_SHIFT + LEDS_LIMIT_TOTAL_SHIFT);
}
