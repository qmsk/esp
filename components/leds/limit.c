#include "limit.h"

#include <logging.h>

#include <stdlib.h>

int leds_limit_init(struct leds_limit *limit, unsigned groups, unsigned leds)
{
  if (!(limit->group_multipliers = calloc(groups, sizeof(*limit->group_multipliers))) && groups) {
    LOG_ERROR("calloc");
    return -1;
  }

  limit->group_count = groups;
  limit->group_size = groups ? leds / groups : 0;

  limit->total_multipler = (1 << LEDS_LIMIT_TOTAL_SHIFT);

  for (unsigned i = 0; i < groups; i++) {
    limit->group_multipliers[i] = (1 << LEDS_LIMIT_GROUP_SHIFT);
  }

  return 0;
}

unsigned leds_limit_set_group(struct leds_limit *limit, unsigned group, unsigned group_limit, unsigned power)
{
  uint16_t multiplier;

  if (power > group_limit) {
    multiplier = (group_limit << LEDS_LIMIT_GROUP_SHIFT) / power;
  } else {
    // unity
    multiplier = (1 << LEDS_LIMIT_GROUP_SHIFT);
  }

  limit->group_multipliers[group] = multiplier;

  return (power * multiplier) >> LEDS_LIMIT_GROUP_SHIFT;
}

unsigned leds_limit_set_total(struct leds_limit *limit, unsigned total_limit, unsigned power)
{
  uint16_t multiplier;

  if (power > total_limit) {
    multiplier = (total_limit << LEDS_LIMIT_TOTAL_SHIFT) / power;
  } else {
    // unity
    multiplier = (1 << LEDS_LIMIT_TOTAL_SHIFT);
  }

  limit->total_multipler = multiplier;

  return (power * multiplier) >> LEDS_LIMIT_TOTAL_SHIFT;
}
