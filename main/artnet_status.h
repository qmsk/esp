#pragma once

#include <artnet.h>
#include <stats_timer.h>
#include <stats_counter.h>

struct artnet_status_stats {
  struct stats_timer recv_timer;
  struct stats_counter recv_poll_counter;
  struct stats_counter recv_dmx_counter;
  struct stats_counter recv_sync_counter;
  struct stats_counter dmx_discard_counter;
};

struct artnet_status_metrics {
  struct stats_timer_metrics recv_timer;
  struct stats_counter_metrics recv_poll_counter;
  struct stats_counter_metrics recv_dmx_counter;
  struct stats_counter_metrics recv_sync_counter;
  struct stats_counter_metrics dmx_discard_counter;
};

struct artnet_status {
    bool sync_mode;

    struct artnet_status_metrics metrics;
};

struct artnet_status get_artnet_status(struct artnet *artnet);
