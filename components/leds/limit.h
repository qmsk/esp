#pragma once

/* Use integer math to scale LED channel values */
struct leds_limit {
  uint16_t multiplier;
  uint8_t shift;
};

static inline struct leds_limit leds_limit_unity()
{
  // no-op (x * 1) >> 0 => x
  return (struct leds_limit) { 1, 0 };
}

static inline struct leds_limit leds_limit_total(unsigned limit, unsigned total)
{
  if (total > limit) {
    // very simple integer math using a fixed /1024 shift
    return (struct leds_limit) {
      .multiplier = limit * (1 << 10) / total,
      .shift      = 10,
    };
  } else {
    // do not limit
    return (struct leds_limit) {
      .multiplier = 1,
      .shift      = 0,
    };
  }
}

static inline bool is_leds_limit(struct leds_limit limit)
{
  return limit.multiplier != 1 || limit.shift > 0;
}

static inline uint8_t leds_limit_uint8(struct leds_limit limit, uint8_t value)
{
  return (value * limit.multiplier) >> limit.shift;
}
