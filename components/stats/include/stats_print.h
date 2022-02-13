#pragma once

#include <stats.h>

#include <stdio.h>

/* Print using a `Title : Desc   ....` format */
void print_stats_timer(const char *title, const char *desc, const struct stats_timer *timer);
void print_stats_counter(const char *title, const char *desc, const struct stats_counter *counter);
void print_stats_gauge(const char *title, const char *desc, const struct stats_gauge *gauge);
