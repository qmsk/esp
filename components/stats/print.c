#include <stats_print.h>

#include <stdio.h>

void print_stats_timer(const char *title, const char *desc, const struct stats_timer *timer)
{
  printf("\t%10s : %-10s %12.3fs total / %6u count @ %12.3fs = %6.1f/s @ %6.1fms avg, %5.1f%% util\n", title, desc,
    (float)(timer->total) / 1000.0f / 1000.0f,
    timer->count,
    stats_timer_seconds_passed(timer),
    stats_timer_average_rate(timer),
    stats_timer_average_seconds(timer) * 1000.0f,
    stats_timer_utilization(timer) * 100.0f
  );
}

void print_stats_counter(const char *title, const char *desc, const struct stats_counter *counter)
{
  printf("\t%10s : %-10s %8u count @ %12.3fs = %6.1f/s avg\n", title, desc,
    counter->count,
    stats_counter_seconds_passed(counter),
    counter->count / stats_counter_seconds_passed(counter)
  );
}

void print_stats_gauge(const char *title, const char *desc, const struct stats_gauge *gauge)
{
  printf("\t%10s : %-10s %8u @ %12.3fs = %8u min - %8u max \n", title, desc,
    gauge->value,
    stats_gauge_seconds_passed(gauge),
    gauge->min,
    gauge->max
  );
}
