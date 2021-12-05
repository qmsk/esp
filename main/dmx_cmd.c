#include "dmx.h"

#include <logging.h>

#include <stdlib.h>

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

  if (output >= DMX_OUTPUT_COUNT) {
    LOG_ERROR("output=%u does not exist", output);
    return CMD_ERR_ARGV;
  }

  struct dmx_output_state *state = &dmx_output_states[output];

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

  if (output >= DMX_OUTPUT_COUNT) {
    LOG_ERROR("output=%u does not exist", output);
    return CMD_ERR_ARGV;
  }

  struct dmx_output_state *state = &dmx_output_states[output];

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

const struct cmd dmx_commands[] = {
  { "zero",   dmx_cmd_zero,       .usage = "COUNT",           .describe = "Output COUNT channels at zero on all output" },
  { "all",    dmx_cmd_all,        .usage = "COUNT VALUE",     .describe = "Output COUNT channels at VALUE on all outputs" },
  { "out",    dmx_cmd_out,        .usage = "OUTPUT VALUE...", .describe = "Output given VALUEs as channels on output" },
  { "count",  dmx_cmd_count,      .usage = "OUTPUT COUNT",    .describe = "Output COUNT channels with 0..COUNT as value" },
  { }
};

const struct cmdtab dmx_cmdtab = {
  .commands = dmx_commands,
};
