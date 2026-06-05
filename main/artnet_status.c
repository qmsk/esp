#include "artnet_status.h"
#include <artnet_stats.h>

struct artnet_status_stats artnet_status_stats = {};
struct artnet_status_metrics artnet_status_metrics = {};

static struct stats_timer get_artnet_recv_timer(struct artnet *artnet)
{
  struct artnet_stats stats;

  artnet_get_stats(artnet, &stats);

  return stats.recv;
}

// basic moving average
static void update_stats_timer_metrics(struct stats_timer *baseline, const struct stats_timer *timer, struct stats_timer_metrics *avg)
{
  struct stats_timer_metrics metrics = stats_timer_diff_metrics(baseline, timer);

  *avg = stats_timer_metrics_average(avg, &metrics);

  *baseline = *timer;
}

void update_artnet_status(struct artnet *artnet)
{
  struct stats_timer recv_timer = get_artnet_recv_timer(artnet);

  update_stats_timer_metrics(&artnet_status_stats.recv_timer, &recv_timer, &artnet_status_metrics.recv_timer);
}

struct artnet_status get_artnet_status(struct artnet *artnet)
{
    update_artnet_status(artnet);

    return (struct artnet_status) {
        .sync_mode  = artnet_is_sync_state(artnet),
        .metrics    = artnet_status_metrics,
    };
}
