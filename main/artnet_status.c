#include "artnet_status.h"
#include <artnet_stats.h>

struct artnet_status_stats artnet_status_stats = {};
struct artnet_status_metrics artnet_status_metrics = {};

// basic moving average
static void update_stats_timer_metrics(struct stats_timer *baseline, const struct stats_timer *timer, struct stats_timer_metrics *avg)
{
  struct stats_timer_metrics metrics = stats_timer_diff_metrics(baseline, timer);

  *avg = stats_timer_metrics_average(avg, &metrics);

  *baseline = *timer;
}

static void update_stats_counter_metrics(struct stats_counter *baseline, const struct stats_counter *counter, struct stats_counter_metrics *avg)
{
  struct stats_counter_metrics metrics = stats_counter_diff_metrics(baseline, counter);

  *avg = stats_counter_metrics_average(avg, &metrics);

  *baseline = *counter;
}

void update_artnet_status(struct artnet *artnet)
{
  struct artnet_stats artnet_stats;
  unsigned artnet_output_coumt = artnet_get_output_count(artnet);

  artnet_get_stats(artnet, &artnet_stats);

  update_stats_timer_metrics(&artnet_status_stats.recv_timer, &artnet_stats.recv, &artnet_status_metrics.recv_timer);
  update_stats_counter_metrics(&artnet_status_stats.recv_poll_counter, &artnet_stats.recv_poll, &artnet_status_metrics.recv_poll_counter);
  update_stats_counter_metrics(&artnet_status_stats.recv_dmx_counter, &artnet_stats.recv_dmx, &artnet_status_metrics.recv_dmx_counter);
  update_stats_counter_metrics(&artnet_status_stats.recv_sync_counter, &artnet_stats.recv_sync, &artnet_status_metrics.recv_sync_counter);
  update_stats_counter_metrics(&artnet_status_stats.dmx_discard_counter, &artnet_stats.dmx_discard, &artnet_status_metrics.dmx_discard_counter);

  for (unsigned i = 0; i < artnet_output_coumt; i++) {
    struct artnet_output_stats artnet_output_stats;

    artnet_get_output_stats(artnet, i, &artnet_output_stats);

    update_stats_counter_metrics(&artnet_status_stats.outputs[i].dmx_counter, &artnet_output_stats.dmx_recv, &artnet_status_metrics.outputs[i].dmx_counter);
    update_stats_counter_metrics(&artnet_status_stats.outputs[i].seq_miss_counter, &artnet_output_stats.seq_miss, &artnet_status_metrics.outputs[i].seq_miss_counter);
    update_stats_counter_metrics(&artnet_status_stats.outputs[i].seq_drop_counter, &artnet_output_stats.seq_drop, &artnet_status_metrics.outputs[i].seq_drop_counter);
    update_stats_counter_metrics(&artnet_status_stats.outputs[i].update_counter, &artnet_output_stats.queue_update, &artnet_status_metrics.outputs[i].update_counter);
    update_stats_counter_metrics(&artnet_status_stats.outputs[i].overflow_counter, &artnet_output_stats.queue_overflow, &artnet_status_metrics.outputs[i].overflow_counter);
  }
}

struct artnet_status get_artnet_status(struct artnet *artnet)
{
    update_artnet_status(artnet);

    return (struct artnet_status) {
        .sync_mode  = artnet_is_sync_state(artnet),
        .metrics    = artnet_status_metrics,
    };
}
