#ifndef __ARTNET_STATS_H__
#define __ARTNET_STATS_H__

#include <stats.h>

struct artnet_stats {
  /* Complete recv -> send packet handling */
  struct stats_timer recv;

  /* Failed to receive ArtNet packet. */
  struct stats_counter recv_error;

  /* Received ArtPoll packets */
  struct stats_counter recv_poll;

  /* Received ArtDmx packets */
  struct stats_counter recv_dmx;

  /* Received ArtSync packets */
  struct stats_counter recv_sync;

  /* Received ArtPoll packets */
  struct stats_counter recv_unknown;

  /* Received packets, rejected as invalid */
  struct stats_counter recv_invalid;

  /* Failed to handle received packet */
  struct stats_counter errors;

  /* Discarded ArtDmx packets, no output found */
  struct stats_counter dmx_discard;

};

struct artnet_input_stats {
  /* Received ArtDMX packets */
  struct stats_counter dmx_recv;

  /* Output queue overflowed, previous packet overwritten */
  struct stats_counter queue_overwrite;
};

struct artnet_output_stats {
  /* Received ArtSync packets */
  struct stats_counter sync_recv;

  /* Received ArtDMX packets */
  struct stats_counter dmx_recv;

  /* Received ArtDMX packets in sync mode */
  struct stats_counter dmx_sync;

  /* Received ArtDMX packets with seq larger than expected */
  struct stats_counter seq_skip;

  /* Dropped ArtDMX packets with seq smaller than expected */
  struct stats_counter seq_drop;

  /* Received ArtDMX packets with seq resynced after timeout */
  struct stats_counter seq_resync;

  /* Output queue overflowed, previous packet overwritten */
  struct stats_counter queue_overwrite;
};

/*
 * Reset all artnet stats
 */
void artnet_reset_stats(struct artnet *artnet);

/*
 * Copy stats for artnet.
 */
void artnet_get_stats(struct artnet *artnet, struct artnet_stats *stats);

/*
 * Copy stats for input.
 *
 * @param artnet
 * @param index input index (see artnet_get_input_count)
 * @param stats input stats (copied)
 *
 * @return <0 on error, 0 if ok, >0 if index invalid.
 */
int artnet_get_input_stats(struct artnet *artnet, int index, struct artnet_input_stats *stats);

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
