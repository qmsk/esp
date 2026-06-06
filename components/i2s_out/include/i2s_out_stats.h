#pragma once

#include <stats_timer.h>

struct i2s_out;

struct i2s_out_stats {
    struct stats_timer out_timer;
};

void i2s_out_reset_stats(struct i2s_out *i2s_out);

struct i2s_out_stats i2s_out_stats(struct i2s_out *i2s_out);
