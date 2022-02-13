#pragma once

struct stats_gauge {
  unsigned val;
  unsigned min, max;
};

static inline void stats_gauge_init(struct stats_gauge *gauge)
{
  gauge->val = 0;
  gauge->min = -1;
  gauge->max = 0;
}

static inline struct stats_gauge stats_gauge_copy(const struct stats_gauge *gauge)
{
  // TODO: locking for concurrent stats updates
  return *gauge;
}

static inline void stats_gauge_sample(struct stats_gauge *gauge, unsigned val)
{
  gauge->val = val;

  if (val < gauge->min) {
    gauge->min = val;
  }

  if (val > gauge->max) {
    gauge->max = val;
  }
}
