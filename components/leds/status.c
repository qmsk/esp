#include <leds_status.h>
#include "leds.h"

void leds_get_limit_total_status(struct leds *leds, struct leds_limit_status *total_status)
{
  *total_status = leds->limit_total_status;
}

void leds_get_limit_groups_status(struct leds *leds, struct leds_limit_status *group_status, size_t *size)
{
  unsigned i;

  for (i = 0; i < *size && i < leds->limit.group_count; i++) {
    group_status[i] = leds->limit_groups_status[i];
  }

  *size = i;
}
