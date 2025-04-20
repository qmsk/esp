#include "dmx.h"
#include "dmx_config.h"
#include "dmx_state.h"

#include <artnet.h>
#include <dmx_input_stats.h>
#include <dmx_output_stats.h>
#include <logging.h>
#include <stats_print.h>

#include <stdlib.h>
#include <string.h>

int dmx_cmd_info(int argc, char **argv, void *ctx)
{
  TickType_t tick = xTaskGetTickCount();

  // inputs
  const struct dmx_input_config *input_config = &dmx_input_config;
  struct dmx_input_state *input_state = &dmx_input_state;

  printf("dmx-input:\n");
  printf("\t%-20s: %s\n", "Enabled", input_config->enabled ? "true" : "false");
  printf("\t%-20s: %s\n", "Initialized", input_state->dmx_input ? "true" : "false");

  if (input_state->artnet_input) {
    const struct artnet_input_options *input_options = artnet_input_options(input_state->artnet_input);
    struct artnet_input_state artnet_state = artnet_input_state(input_state->artnet_input);
  
    printf("\t%-20s: \n", "Art-Net");
    printf("\t\t%-20s: net %u sub-net %u universe %u\n", "Address",
      artnet_address_net(input_options->address),
      artnet_address_subnet(input_options->address),
      artnet_address_universe(input_options->address)
    );
    printf("\t\t%-20s: %u\n", "Length", artnet_state.len);
    printf("\t\t%-20s: %d ms\n", "Tick", artnet_state.tick ? (tick - artnet_state.tick) * portTICK_RATE_MS : 0);
  }

  // outputs
  for (int i = 0; i < DMX_OUTPUT_COUNT; i++) {
    const struct dmx_output_config *output_config = &dmx_output_configs[i];
    struct dmx_output_state *output_state = &dmx_output_states[i];

    printf("dmx-output%d:\n", i + 1);
    printf("\t%-20s: %s\n", "Enabled", output_config->enabled ? "true" : "false");
    printf("\t%-20s: %s\n", "Initialized", output_state->dmx_output ? "true" : "false");

    if (output_state->artnet_output) {
      const struct artnet_output_options *output_options = artnet_output_options(output_state->artnet_output);
      struct artnet_output_state artnet_state = artnet_output_state(output_state->artnet_output);
    
      printf("\t%-20s: \n", "Art-Net");
      printf("\t\t%-20s: net %u sub-net %u universe %u\n", "Address",
        artnet_address_net(output_options->address),
        artnet_address_subnet(output_options->address),
        artnet_address_universe(output_options->address)
      );
      printf("\t\t%-20s: %u\n", "Seq", artnet_state.seq);
      printf("\t\t%-20s: %d ms\n", "Tick", artnet_state.tick ? (tick - artnet_state.tick) * portTICK_RATE_MS : 0);
    }
  }

  return 0;
}
int dmx_cmd_zero(int argc, char **argv, void *ctx)
{
  int err;
  int count;
  uint8_t *data;

  if ((err = cmd_arg_int(argc, argv, 1, &count)))
    return err;

  if (!(data = malloc(count))) {
    LOG_ERROR("malloc");
  }

  for (int i = 0; i < count; i++) {
    data[i] = 0;
  }

  for (int i = 0; i < DMX_OUTPUT_COUNT; i++)
  {
    struct dmx_output_state *state = &dmx_output_states[i];

    if (!state->dmx_output) {
      continue;
    }

    if ((err = output_dmx(state, data, count))) {
      LOG_ERROR("output_dmx");
      goto error;
    }
  }

error:
  free(data);

  return err;
}

int dmx_cmd_all(int argc, char **argv, void *ctx)
{
  int err;
  int count, value;
  uint8_t *data;

  if ((err = cmd_arg_int(argc, argv, 1, &count)))
    return err;
  if ((err = cmd_arg_int(argc, argv, 2, &value)))
    return err;

  if (!(data = malloc(count))) {
    LOG_ERROR("malloc");
  }

  for (int i = 0; i < count; i++) {
    data[i] = value;
  }

  for (int i = 0; i < DMX_OUTPUT_COUNT; i++)
  {
    struct dmx_output_state *state = &dmx_output_states[i];

    if (!state->dmx_output) {
      continue;
    }

    if ((err = output_dmx(state, data, count))) {
      LOG_ERROR("output_dmx");
      goto error;
    }
  }

error:
  free(data);

  return err;
}

int dmx_cmd_out(int argc, char **argv, void *ctx)
{
  int err;
  int output;
  int count = argc - 2;
  uint8_t *data;

  if ((err = cmd_arg_int(argc, argv, 1, &output)))
    return err;

  if (output <= 0 || output > DMX_OUTPUT_COUNT) {
    LOG_ERROR("output=%u does not exist", output);
    return CMD_ERR_ARGV;
  }

  struct dmx_output_state *state = &dmx_output_states[output - 1];

  if (!state->dmx_output) {
    LOG_WARN("output=%u is not enabled", output);
    return 0;
  }

  if (!(data = malloc(count))) {
    LOG_ERROR("malloc");
    return -1;
  }

  for (int i = 0; i < count; i++) {
    int value;

    if ((err = cmd_arg_int(argc, argv, i + 2, &value)))
      goto error;

    data[i] = value;
  }

  if ((err = output_dmx(state, data, count))) {
    LOG_ERROR("output_dmx");
    goto error;
  }

error:
  free(data);

  return err;
}

int dmx_cmd_count(int argc, char **argv, void *ctx)
{
  int err;
  int output;
  int count;
  uint8_t *data;

  if ((err = cmd_arg_int(argc, argv, 1, &output)))
    return err;
  if ((err = cmd_arg_int(argc, argv, 2, &count)))
    return err;

  if (output <= 0 || output > DMX_OUTPUT_COUNT) {
    LOG_ERROR("output=%u does not exist", output);
    return CMD_ERR_ARGV;
  }

  struct dmx_output_state *state = &dmx_output_states[output - 1];

  if (!state->dmx_output) {
    LOG_WARN("output=%u is not enabled", output);
    return 0;
  }

  if (!(data = malloc(count))) {
    LOG_ERROR("malloc");
  }

  for (int i = 0; i < count; i++) {
    data[i] = (uint8_t) i;
  }

  if ((err = output_dmx(state, data, count))) {
    LOG_ERROR("output_dmx");
    goto error;
  }

error:
  free(data);

  return err;
}

int dmx_cmd_stats(int argc, char **argv, void *ctx)
{
  struct dmx_input *dmx_input = dmx_input_state.dmx_input;
  bool reset = false;

  if (argc > 1 && strcmp(argv[1], "reset") == 0) {
    reset = true;
  } 

  if (dmx_input) {
    struct dmx_input_stats stats;

    dmx_input_stats(dmx_input, &stats, reset);

    printf("Input:\n");

    print_stats_timer  ("UART",   "Open",     &stats.uart_open);
    print_stats_timer  ("UART",   "RX",       &stats.uart_rx);
    printf("\t\n");
    print_stats_counter("RX",     "overflow", &stats.rx_overflow);
    print_stats_counter("RX",     "error",    &stats.rx_error);
    print_stats_counter("RX",     "break",    &stats.rx_break);
    print_stats_counter("RX",     "desync",   &stats.rx_desync);
    printf("\t\n");
    print_stats_counter("DMX",    "dimmer",   &stats.cmd_dimmer);
    print_stats_counter("DMX",    "unknown",  &stats.cmd_unknown);
    printf("\t\n");
    print_stats_gauge(  "Data",   "len",      &stats.data_len);
    printf("\n");
  }

  for (unsigned i = 0; i < DMX_OUTPUT_COUNT; i++) {
    struct dmx_output *dmx_output = dmx_output_states[i].dmx_output;
    struct dmx_output_stats stats;

    if (!dmx_output) {
      continue;
    }

    dmx_output_stats(dmx_output, &stats, reset);

    printf("Output %d:\n", i + 1);

    print_stats_timer  ("UART",   "Open",     &stats.uart_open);
    print_stats_timer  ("UART",   "TX",       &stats.uart_tx);
    printf("\t\n");
    print_stats_counter("TX",     "error",    &stats.tx_error);
    printf("\t\n");
    print_stats_counter("DMX",    "dimmer",   &stats.cmd_dimmer);
    printf("\t\n");
    print_stats_gauge(  "Data",   "len",      &stats.data_len);
    printf("\n");
  }

  return 0;
}

const struct cmd dmx_commands[] = {
  { "info",   dmx_cmd_info,                                   .describe = "Show input/output configuration state" },
  { "zero",   dmx_cmd_zero,       .usage = "COUNT",           .describe = "Output COUNT channels at zero on all output" },
  { "all",    dmx_cmd_all,        .usage = "COUNT VALUE",     .describe = "Output COUNT channels at VALUE on all outputs" },
  { "out",    dmx_cmd_out,        .usage = "OUTPUT VALUE...", .describe = "Output given VALUEs as channels on output" },
  { "count",  dmx_cmd_count,      .usage = "OUTPUT COUNT",    .describe = "Output COUNT channels with 0..COUNT as value" },
  { "stats",  dmx_cmd_stats,      .usage = "[reset]",         .describe = "Show input/output stats" },
  { }
};

const struct cmdtab dmx_cmdtab = {
  .commands = dmx_commands,
};
