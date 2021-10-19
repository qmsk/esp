#ifndef __ARTNET_STATS_H__
#define __ARTNET_STATS_H__

#include <stats.h>

struct artnet_stats {
  /* Failed to receive ArtNet packet. */
  struct stats_counter recv_error;

  /* Received ArtPoll packets */
  struct stats_counter recv_poll;

  /* Received ArtDmx packets */
  struct stats_counter recv_dmx;

  /* Received ArtPoll packets */
  struct stats_counter recv_unknown;

  /* Received packets, rejected as invalid */
  struct stats_counter recv_invalid;

  /* Failed to handle received packet */
  struct stats_counter errors;

  /* Discarded ArtDmx packets, no output found */
  struct stats_counter dmx_discard;

};

struct artnet_output_stats {
  /* Received ArtDMX packets */
  struct stats_counter dmx_recv;

  /* Received ArtDMX packets with seq larger than expected */
  struct stats_counter seq_skip;

  /* Dropped ArtDMX packets with seq smaller than expected */
  struct stats_counter seq_drop;

  /* Output queue overflowed, previous packet overwritten */
  struct stats_counter queue_overwrite;
};

/*
 * Copy stats for artnet.
 */
 void artnet_get_stats(struct artnet *artnet, struct artnet_stats *stats);

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
