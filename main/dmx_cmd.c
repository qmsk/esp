#include "dmx.h"
#include "dmx_config.h"
#include "dmx_state.h"

#include <dmx_input_stats.h>
#include <logging.h>
#include <stats_print.h>

#include <stdlib.h>

int dmx_cmd_info(int argc, char **argv, void *ctx)
{
  // inputs
  const struct dmx_input_config *input_config = &dmx_input_config;
  struct dmx_input_state *input_state = &dmx_input_state;

  printf("dmx-input:\n");
  printf("\t%-20s: %s\n", "Enabled", input_config->enabled ? "true" : "false");
  printf("\t%-20s: %s\n", "Initialized", input_state->dmx_input ? "true" : "false");

  // outputs
  for (int i = 0; i < DMX_OUTPUT_COUNT; i++) {
    const struct dmx_output_config *output_config = &dmx_output_configs[i];
    struct dmx_output_state *output_state = &dmx_output_states[i];

    printf("dmx-output%d:\n", i + 1);
    printf("\t%-20s: %s\n", "Enabled", output_config->enabled ? "true" : "false");
    printf("\t%-20s: %s\n", "Initialized", output_state->dmx_output ? "true" : "false");

    if (!output_config->enabled || !output_state->dmx_output) {
      continue;
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
  if (dmx_input_state.dmx_input) {
    struct dmx_input_stats stats;

    dmx_input_stats(dmx_input_state.dmx_input, &stats);

    printf("Input:\n");

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

  return 0;
}

const struct cmd dmx_commands[] = {
  { "info",   dmx_cmd_info,                                   .describe = "Show input/output configuration state" },
  { "zero",   dmx_cmd_zero,       .usage = "COUNT",           .describe = "Output COUNT channels at zero on all output" },
  { "all",    dmx_cmd_all,        .usage = "COUNT VALUE",     .describe = "Output COUNT channels at VALUE on all outputs" },
  { "out",    dmx_cmd_out,        .usage = "OUTPUT VALUE...", .describe = "Output given VALUEs as channels on output" },
  { "count",  dmx_cmd_count,      .usage = "OUTPUT COUNT",    .describe = "Output COUNT channels with 0..COUNT as value" },
  { "stats",  dmx_cmd_stats,                                 .describe = "Show input/output stats" },
  { }
};

const struct cmdtab dmx_cmdtab = {
  .commands = dmx_commands,
};
