#include "artnet.h"
#include "artnet_state.h"

#include <artnet.h>
#include "artnet_config.h"
#include <artnet_stats.h>
#include <logging.h>
#include <stats_print.h>

#include <string.h>

int artnet_cmd_info(int argc, char **argv, void *ctx)
{
  struct artnet_input_options artnet_input_options[ARTNET_INPUTS_MAX];
  struct artnet_output_options artnet_output_options[ARTNET_OUTPUTS_MAX];

  if (!artnet) {
    LOG_WARN("artnet disabled");
    return 1;
  }

  struct artnet_options options = artnet_get_options(artnet);
  unsigned input_count = artnet_get_input_count(artnet);
  unsigned output_count = artnet_get_output_count(artnet);
  size_t inputs_size = ARTNET_INPUTS_MAX, outputs_size = ARTNET_OUTPUTS_MAX;
  int err;

  printf("Config:\n");
  printf("\tIPv4: address=%u.%u.%u.%u\n",
    options.metadata.ip_address[0],
    options.metadata.ip_address[1],
    options.metadata.ip_address[2],
    options.metadata.ip_address[3]
  );
  printf("\tMAC: address=%02x:%02x:%02x:%02x:%02x:%02x\n",
    options.metadata.mac_address[0],
    options.metadata.mac_address[1],
    options.metadata.mac_address[2],
    options.metadata.mac_address[3],
    options.metadata.mac_address[4],
    options.metadata.mac_address[5]
  );
  printf("\tUDP: port=%u\n", options.port);
  printf("\tShort name: %s\n", options.metadata.short_name);
  printf("\tLong name: %s\n", options.metadata.long_name);

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

  for (int i = 0; i < inputs_size && i < ARTNET_INPUTS_MAX; i++) {
    struct artnet_input_options *options = &artnet_input_options[i];
    struct artnet_input_state state;
    TickType_t tick = xTaskGetTickCount();

    if ((err = artnet_get_input_state(artnet, i, &state))) {
      LOG_ERROR("artnet_get_input_state");
      continue;
    }

    printf("\t%2d: net %3u subnet %2u universe %2u <- %-16.16s: len %3u @ %d ms\n", i,
      artnet_address_net(options->address), artnet_address_subnet(options->address), artnet_address_universe(options->address),
      options->name,
      state.len,
      state.tick ? (tick - state.tick) * portTICK_RATE_MS : 0
    );
  }

  printf("\n");
  printf("Outputs: count=%u\n", output_count);

  for (int i = 0; i < outputs_size && i < ARTNET_OUTPUTS_MAX; i++) {
    struct artnet_output_options *options = &artnet_output_options[i];
    struct artnet_output_state state;
    TickType_t tick = xTaskGetTickCount();

    if ((err = artnet_get_output_state(artnet, i, &state))) {
      LOG_ERROR("artnet_get_output_state");
      continue;
    }

    printf("\t%2d: net %3u subnet %2u universe %2u -> %-16.16s: seq %3u @ %d ms\n", i,
      artnet_address_net(options->address),
      artnet_address_subnet(options->address),
      artnet_address_universe(options->address),
      options->name,
      state.seq,
      state.tick ? (tick - state.tick) * portTICK_RATE_MS : 0
    );
  }

  return 0;
}

int artnet_cmd_stats(int argc, char **argv, void *ctx)
{
  if (!artnet) {
    LOG_WARN("artnet disabled");
    return 1;
  }

  if (argc > 1 && strcmp(argv[1], "reset") == 0) {
    LOG_INFO("reset artnet stats");

    artnet_reset_stats(artnet);
  }

  // network stats
  struct artnet_stats stats;

  artnet_get_stats(artnet, &stats);

  printf("Art-Net: \n");

  print_stats_timer  ("Network",  "receive",    &stats.recv);

  print_stats_counter("Poll",     "received",   &stats.recv_poll);
  print_stats_counter("DMX",      "received",   &stats.recv_dmx);
  print_stats_counter("DMX",      "discarded",  &stats.dmx_discard);
  print_stats_counter("Sync",     "received",   &stats.recv_sync);
  print_stats_counter("Unknown",  "received",   &stats.recv_unknown);
  print_stats_counter("Recv",     "errors",     &stats.recv_error);
  print_stats_counter("Recv",     "invalid",    &stats.recv_invalid);
  print_stats_counter("Errors",   "",           &stats.errors);

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

    print_stats_counter("DMX",    "received",   &input_stats.dmx_recv);
    print_stats_counter("Queue",  "overflowed", &input_stats.queue_overwrite);

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

    print_stats_counter("Sync",   "received",   &output_stats.sync_recv);
    print_stats_counter("DMX",    "received",   &output_stats.dmx_recv);
    print_stats_counter("DMX",    "synced",     &output_stats.dmx_sync);
    print_stats_counter("Seq",    "skipped",    &output_stats.seq_skip);
    print_stats_counter("Seq",    "dropped",    &output_stats.seq_drop);
    print_stats_counter("Seq",    "resynced",   &output_stats.seq_resync);
    print_stats_counter("Queue",  "overflowed", &output_stats.queue_overwrite);

    printf("\n");
  }

  return 0;
}

int artnet_cmd_sync(int argc, char **argv, void *ctx)
{
  int err;

  if (!artnet) {
    LOG_ERROR("disabled");
    return -1;
  }

  if ((err = artnet_sync_outputs(artnet))) {
    LOG_ERROR("artnet_sync_outputs");
    return err;
  }

  return 0;
}

const struct cmd artnet_commands[] = {
  { "info",      artnet_cmd_info,     .usage = "",                      .describe = "Show configuration" },
  { "stats",     artnet_cmd_stats,    .usage = "[reset]",               .describe = "Show/reset receiver and output stats" },
  { "sync",      artnet_cmd_sync,     .usage = "",                      .describe = "Force output sync" },
  { }
};

const struct cmdtab artnet_cmdtab = {
  .commands = artnet_commands,
};
