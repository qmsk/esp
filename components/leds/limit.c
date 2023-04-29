#include "leds.h"
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

  if (power > total_limit && total_limit) {
    multiplier = (total_limit << LEDS_LIMIT_TOTAL_SHIFT) / power;
  } else {
    // unity
    multiplier = (1 << LEDS_LIMIT_TOTAL_SHIFT);
  }

  limit->total_multipler = multiplier;

  return (power * multiplier) >> LEDS_LIMIT_TOTAL_SHIFT;
}

void leds_limit_update(struct leds *leds)
{
  unsigned total_power = 0;

  if (!leds->pixels_limit_dirty) {
    return; // already up to date
  }

  if (leds->limit.group_count && leds->options.limit_group) {
    for (unsigned group = 0; group < leds->limit.group_count; group++) {
      unsigned count = leds->limit.group_size;
      unsigned index = leds->limit.group_size * group;
      unsigned group_power = leds_power_total(leds->pixels, index, count, leds->protocol_type->power_mode);
      unsigned output_power = leds_limit_set_group(&leds->limit, group, leds->options.limit_group, group_power);

      total_power += output_power;

      LOG_DEBUG("group[%u] limit=%u power=%u -> output power=%u", group,
        leds->options.limit_group,
        group_power,
        output_power
      );

      leds->limit_groups_status[group] = (struct leds_limit_status) {
        .count  = count,
        .limit  = leds->options.limit_group,
        .power  = group_power,
        .output = output_power,
      };
    }
  } else {
    total_power = leds_power_total(leds->pixels, 0, leds->options.count, leds->protocol_type->power_mode);
  }

  // apply total limit
  unsigned output_power = leds_limit_set_total(&leds->limit, leds->options.limit_total, total_power);

  LOG_DEBUG("total limit=%u power=%u -> output power=%u",
    leds->options.limit_total,
    total_power,
    output_power
  );

  leds->limit_total_status = (struct leds_limit_status) {
    .count  = leds->options.count,
    .limit  = leds->options.limit_total,
    .power  = total_power,
    .output = output_power,
  };

  leds->pixels_limit_dirty = false;
}

void leds_get_limit_total_status(struct leds *leds, struct leds_limit_status *total_status)
{
  leds_limit_update(leds);

  *total_status = leds->limit_total_status;
}

void leds_get_limit_groups_status(struct leds *leds, struct leds_limit_status *group_status, size_t *size)
{
  unsigned i;

  leds_limit_update(leds);

  for (i = 0; i < *size && i < leds->limit.group_count; i++) {
    group_status[i] = leds->limit_groups_status[i];
  }

  *size = i;
}
