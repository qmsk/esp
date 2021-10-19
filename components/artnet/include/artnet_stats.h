#ifndef __ARTNET_STATS_H__
#define __ARTNET_STATS_H__

#include <stats.h>

struct artnet_output_stats {
  /* Received ArtDMX packets */
  struct stats_counter recv;

  /* Dropped ArtDMX packets due to seq mismatch */
  struct stats_counter seq_drop;

  /* Output queue overflowed, previous packet dropped */
  struct stats_counter overflow_drop;
};

/*
 * Copy stats for output.
 *
 * @param artnet
 * @param index output index (see artnet_get_output_count)
 * @param stats output stats (copied)
 *
 * @return <0 on error, 0 if ok, >0 if index invalid.
 */
int artnet_get_output_stats(struct artnet *artnet, int index, struct artnet_output_stats *stats);


#endif
