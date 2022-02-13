#include "artnet.h"
#include "artnet_state.h"

#include <artnet.h>
#include <artnet_stats.h>
#include <logging.h>

#define ARTNET_INPUT_COUNT 4
#define ARTNET_OUTPUT_COUNT 16

int artnet_cmd_info(int argc, char **argv, void *ctx)
{
  struct artnet_input_options artnet_input_options[ARTNET_INPUT_COUNT];
  struct artnet_output_options artnet_output_options[ARTNET_OUTPUT_COUNT];

  if (!artnet) {
    LOG_WARN("artnet disabled");
    return 1;
  }

  struct artnet_options options = artnet_get_options(artnet);
  unsigned input_count = artnet_get_input_count(artnet);
  unsigned output_count = artnet_get_output_count(artnet);
  size_t inputs_size = ARTNET_INPUT_COUNT, outputs_size = ARTNET_OUTPUT_COUNT;
  int err;

  printf("Listen port=%u\n", options.port);
  printf("Address net=%u subnet=%u\n", artnet_address_net(options.address), artnet_address_subnet(options.address));
  printf("Metadata:\n");
  printf("\tNetwork IPv4=%u.%u.%u.%u MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
    options.metadata.ip_address[0], options.metadata.ip_address[1], options.metadata.ip_address[2], options.metadata.ip_address[3],
    options.metadata.mac_address[0], options.metadata.mac_address[1], options.metadata.mac_address[2], options.metadata.mac_address[3], options.metadata.mac_address[4], options.metadata.mac_address[5]
  );
  printf("\tName short=%s long=%s\n", options.metadata.short_name, options.metadata.long_name);

  if ((err = artnet_get_inputs(artnet, artnet_input_options, &inputs_size))) {
    LOG_ERROR("artnet_get_inputs");
    return err;
  }

  if ((err = artnet_get_outputs(artnet, artnet_output_options, &outputs_size))) {
    LOG_ERROR("artnet_get_outputs");
    return err;
  }

  printf("\n");
  printf("Inputs: count=%u\n", input_count);

  for (int i = 0; i < inputs_size && i < ARTNET_INPUT_COUNT; i++) {
    struct artnet_input_options *options = &artnet_input_options[i];
    struct artnet_input_state state;

    if ((err = artnet_get_input_state(artnet, i, &state))) {
      LOG_ERROR("artnet_get_input_state");
      continue;
    }

    printf("\t%2d: port=%1d index=%3u @ net %3u subnet %2u universe %2u: len %3u\n", i,
      options->port, options->index,
      artnet_address_net(options->address), artnet_address_subnet(options->address), artnet_address_universe(options->address),
      state.len
    );
  }

  printf("\n");
  printf("Outputs: count=%u\n", output_count);

  for (int i = 0; i < outputs_size && i < ARTNET_OUTPUT_COUNT; i++) {
    struct artnet_output_options *options = &artnet_output_options[i];
    struct artnet_output_state state;

    if ((err = artnet_get_output_state(artnet, i, &state))) {
      LOG_ERROR("artnet_get_output_state");
      continue;
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
  if (!artnet) {
    LOG_WARN("artnet disabled");
    return 1;
  }

  // network stats
  struct artnet_stats stats;

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

  // input stats
  unsigned input_count = artnet_get_input_count(artnet);

  for (int i = 0; i < input_count; i++) {
    struct artnet_input_stats input_stats = {};

    if (artnet_get_input_stats(artnet, i, &input_stats)) {
      LOG_WARN("artnet_get_input_stats index=%d", i);
      continue;
    }

    printf("Input %d: \n", i);

    print_artnet_stats_counter(input_stats.dmx_recv,          "DMX",    "received");
    print_artnet_stats_counter(input_stats.queue_overwrite,   "Queue",  "overflowed");

    printf("\n");
  }

  // output stats
  unsigned output_count = artnet_get_output_count(artnet);

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

int artnet_cmd_test(int argc, char **argv, void *ctx)
{
  int err;

  if (!artnet) {
    LOG_WARN("artnet disabled");
    return 1;
  }

  if ((err = artnet_test_outputs(artnet))) {
    LOG_ERROR("artnet_test_outputs");
    return err;
  }

  return 0;
}

const struct cmd artnet_commands[] = {
  { "info",      artnet_cmd_info,     .usage = "",                      .describe = "Show configuration" },
  { "stats",     artnet_cmd_stats,    .usage = "",                      .describe = "Show receiver and output stats" },
  { "test",      artnet_cmd_test,     .usage = "",                      .describe = "Trigger output test mode" },
  { }
};

const struct cmdtab artnet_cmdtab = {
  .commands = artnet_commands,
};
