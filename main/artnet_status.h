#pragma once

#include <artnet.h>
#include <stats_timer.h>

struct artnet_status_stats {
  struct stats_timer recv_timer;
};

struct artnet_status_metrics {
  struct stats_timer_metrics recv_timer;
};

struct artnet_status {
    bool sync_mode;

    struct artnet_status_metrics metrics;
};

struct artnet_status get_artnet_status(struct artnet *artnet);
