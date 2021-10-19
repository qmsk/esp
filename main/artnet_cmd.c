#include "artnet.h"
#include "artnet_cmd.h"

#include <artnet.h>
#include <artnet_stats.h>
#include <logging.h>

#define ARTNET_OUTPUT_COUNT 16

int artnet_cmd_info(int argc, char **argv, void *ctx)
{
  struct artnet_output_options artnet_output_options[ARTNET_OUTPUT_COUNT];
  struct artnet_options options = artnet_get_options(artnet);
  unsigned output_count = artnet_get_output_count(artnet);
  size_t outputs_size = ARTNET_OUTPUT_COUNT;
  int err;

  printf("Listen port=%u\n", options.port);
  printf("Address net=%u subnet=%u\n", artnet_address_net(options.address), artnet_address_subnet(options.address));
  printf("Network IPv4=%u.%u.%u.%u MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
    options.ip_address[0], options.ip_address[1], options.ip_address[2], options.ip_address[3],
    options.mac_address[0], options.mac_address[1], options.mac_address[2], options.mac_address[3], options.mac_address[4], options.mac_address[5]
  );
  printf("Name short=%s long=%s\n", options.short_name, options.long_name);

  if ((err = artnet_get_outputs(artnet, artnet_output_options, &outputs_size))) {
    LOG_ERROR("artnet_get_outputs");
    return err;
  }

  printf("\n");
  printf("Outputs: count=%u\n", output_count);

  for (int i = 0; i < outputs_size && i < ARTNET_OUTPUT_COUNT; i++) {
    struct artnet_output_options *options = &artnet_output_options[i];
    struct artnet_output_state state;

    if ((err = artnet_get_output_state(artnet, i, &state))) {
      LOG_ERROR("artnet_get_output_state");
      return err;
    }

    printf("\t%2d: port=%1d index=%3u @ net %3u subnet %2u universe %2u -> %16s[%3u]: seq %3u\n", i,
      options->port, options->index,
      artnet_address_net(options->address), artnet_address_subnet(options->address), artnet_address_universe(options->address),
      options->task ? pcTaskGetName(options->task) : "?", options->index,
      state.seq
    );
  }

  return 0;
}

static void print_artnet_stats_counter(struct stats_counter counter, const char *title, const char *desc)
{
  printf("\t%20s : %10s %8u @ %6u.%03us\n", title, desc,
    counter.count,
    stats_counter_milliseconds_passed(&counter) / 1000,
    stats_counter_milliseconds_passed(&counter) % 1000
  );
}

int artnet_cmd_stats(int argc, char **argv, void *ctx)
{
  struct artnet_stats stats;
  unsigned output_count = artnet_get_output_count(artnet);

  // receiver stats
  artnet_get_stats(artnet, &stats);

  printf("Art-Net: \n");

  print_artnet_stats_counter(stats.recv_poll,     "Poll",     "received");
  print_artnet_stats_counter(stats.recv_dmx,      "DMX",      "received");
  print_artnet_stats_counter(stats.dmx_discard,   "DMX",      "discarded");
  print_artnet_stats_counter(stats.recv_sync,     "Sync",     "received");
  print_artnet_stats_counter(stats.recv_unknown,  "Unknown",  "received");
  print_artnet_stats_counter(stats.recv_error,    "Recv",     "errors");
  print_artnet_stats_counter(stats.recv_invalid,  "Recv",     "invalid");
  print_artnet_stats_counter(stats.errors,        "Errors",   "");

  printf("\n");

  // output stats
  for (int i = 0; i < output_count; i++) {
    struct artnet_output_stats output_stats = {};

    if (artnet_get_output_stats(artnet, i, &output_stats)) {
      LOG_WARN("artnet_get_output_stats index=%d", i);
      continue;
    }

    printf("Output %d: \n", i);

    print_artnet_stats_counter(output_stats.sync_recv,        "Sync",   "received");
    print_artnet_stats_counter(output_stats.dmx_recv,         "DMX",    "received");
    print_artnet_stats_counter(output_stats.dmx_sync,         "DMX",    "synced");
    print_artnet_stats_counter(output_stats.seq_skip,         "Seq",    "skipped");
    print_artnet_stats_counter(output_stats.seq_drop,         "Seq",    "dropped");
    print_artnet_stats_counter(output_stats.queue_overwrite,  "Queue",  "overflowed");

    printf("\n");
  }

  return 0;
}

const struct cmd artnet_commands[] = {
  { "info",      artnet_cmd_info,     .usage = "",                      .describe = "Show configuration" },
  { "stats",     artnet_cmd_stats,    .usage = "",                      .describe = "Show receiver and output stats" },
  { }
};

const struct cmdtab artnet_cmdtab = {
  .commands = artnet_commands,
};
