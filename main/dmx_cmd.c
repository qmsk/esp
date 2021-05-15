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

  if ((err = output_dmx(data, count))) {
    LOG_ERROR("output_dmx");
    goto error;
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

  if ((err = output_dmx(data, count))) {
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
  int count;
  uint8_t *data;

  if ((err = cmd_arg_int(argc, argv, 1, &count)))
    return err;

  if (!(data = malloc(count))) {
    LOG_ERROR("malloc");
  }

  for (int i = 0; i < count; i++) {
    data[i] = (uint8_t) i;
  }

  if ((err = output_dmx(data, count))) {
    LOG_ERROR("output_dmx");
    goto error;
  }

error:
  free(data);

  return err;
}

int dmx_cmd_out(int argc, char **argv, void *ctx)
{
  int err;
  int count = argc - 1;
  uint8_t *data;

  if (!(data = malloc(count))) {
    LOG_ERROR("malloc");
  }

  for (int i = 0; i < count; i++) {
    int value;

    if ((err = cmd_arg_int(argc, argv, i + 1, &value)))
      return err;

    data[i] = value;
  }

  if ((err = output_dmx(data, count))) {
    LOG_ERROR("output_dmx");
    goto error;
  }

error:
  free(data);

  return err;
}

const struct cmd dmx_commands[] = {
  { "zero",   dmx_cmd_zero,       .usage = "COUNT",          .describe = "Output COUNT channels at zero" },
  { "all",    dmx_cmd_all,        .usage = "COUNT VALUE",    .describe = "Output COUNT channels at VALUE" },
  { "count",  dmx_cmd_count,      .usage = "COUNT",          .describe = "Output COUNT channels with 0..COUNT as value" },
  { "out",    dmx_cmd_out,        .usage = "VALUE...",       .describe = "Output given VALUEs as channels" },
  { }
};

const struct cmdtab dmx_cmdtab = {
  .commands = dmx_commands,
};
