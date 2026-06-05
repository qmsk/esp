#pragma once

#include <leds_status.h>
#include "leds_config.h"

struct leds_status {
    TickType_t tick;

    bool active;
    TickType_t update_tick;

    bool test;
    enum leds_test_mode test_mode;

    bool artnet;
    TickType_t artnet_dmx_tick;

    struct leds_limit_status limit_total_status;
    struct leds_limit_status limit_groups_status[LEDS_LIMIT_GROUPS_MAX];
    size_t limit_groups_count;
};

void get_leds_status(struct leds_state *leds, struct leds_status *status);
