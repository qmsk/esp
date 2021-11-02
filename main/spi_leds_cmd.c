#include "spi_leds.h"

#include <logging.h>
#include <spi_leds.h>

int spi_leds_cmd_clear(int argc, char **argv, void *ctx)
{
  struct spi_led_color spi_led_color = { }; // off
  int err;

  for (int i = 0; i < SPI_LEDS_COUNT; i++) {
    const struct spi_leds_config *config = &spi_leds_configs[i];
    struct spi_leds_state *state = &spi_leds_states[i];

    if (!config->enabled || !state->spi_leds) {
      continue;
    }

    if ((err = spi_leds_set_all(state->spi_leds, spi_led_color))) {
      LOG_ERROR("spi_leds_set_all");
      return err;
    }

    if ((err = update_spi_leds(state))) {
      LOG_ERROR("update_spi_leds");
      return err;
    }
  }

  return 0;
}

int spi_leds_cmd_all(int argc, char **argv, void *ctx)
{
  int rgb, a = 0xff;
  int err;

  if ((err = cmd_arg_int(argc, argv, 1, &rgb)))
    return err;
  if ((argc > 2) && (err = cmd_arg_int(argc, argv, 2, &a)))
    return err;

  struct spi_led_color spi_led_color = {
    .r = (rgb >> 16) & 0xFF,
    .g = (rgb >>  8) & 0xFF,
    .b = (rgb >>  0) & 0xFF,

    .brightness = a,
  };

  for (int i = 0; i < SPI_LEDS_COUNT; i++) {
    const struct spi_leds_config *config = &spi_leds_configs[i];
    struct spi_leds_state *state = &spi_leds_states[i];

    if (!config->enabled || !state->spi_leds) {
      continue;
    }

    if ((err = spi_leds_set_all(state->spi_leds, spi_led_color))) {
      LOG_ERROR("spi_leds_set_all");
      return err;
    }

    if ((err = update_spi_leds(state))) {
      LOG_ERROR("update_spi_leds");
      return err;
    }
  }

  return 0;
}

int spi_leds_cmd_set(int argc, char **argv, void *ctx)
{
  unsigned output, index;
  int rgb, a = 0xff;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &output)))
    return err;
  if ((err = cmd_arg_uint(argc, argv, 2, &index)))
    return err;
  if ((err = cmd_arg_int(argc, argv, 3, &rgb)))
    return err;
  if ((argc > 4) && (err = cmd_arg_int(argc, argv, 4, &a)))
    return err;

  struct spi_led_color spi_led_color = {
    .r = (rgb >> 16) & 0xFF,
    .g = (rgb >>  8) & 0xFF,
    .b = (rgb >>  0) & 0xFF,

    .brightness = a,
  };

  if (output >= SPI_LEDS_COUNT) {
    LOG_ERROR("output=%u does not exist", output);
    return CMD_ERR_ARGV;
  }

  const struct spi_leds_config *config = &spi_leds_configs[output];
  struct spi_leds_state *state = &spi_leds_states[output];

  if (!config->enabled || !state->spi_leds) {
    LOG_WARN("output=%u is not enabled", output);
    return 0;
  }

  if ((err = spi_leds_set(state->spi_leds, index, spi_led_color))) {
    LOG_ERROR("spi_leds_set");
    return err;
  }

  if ((err = update_spi_leds(state))) {
    LOG_ERROR("update_spi_leds");
    return err;
  }

  return 0;
}

int spi_leds_cmd_test(int argc, char **argv, void *ctx)
{
  unsigned output;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &output)))
    return err;

  if (output >= SPI_LEDS_COUNT) {
    LOG_ERROR("output=%u does not exist", output);
    return CMD_ERR_ARGV;
  }

  const struct spi_leds_config *config = &spi_leds_configs[output];
  struct spi_leds_state *state = &spi_leds_states[output];

  if (!config->enabled || !state->spi_leds) {
    LOG_WARN("output=%u is not enabled", output);
    return 0;
  }

  if ((err = test_spi_leds(state))) {
    LOG_ERROR("test_spi_leds");
    return err;
  }

  return 0;
}

const struct cmd spi_leds_commands[] = {
  { "clear",  spi_leds_cmd_clear, .usage = "",                      .describe = "Clear all output values" },
  { "all",    spi_leds_cmd_all,   .usage = "RGB [A]",               .describe = "Set all output pixels to value" },
  { "set",    spi_leds_cmd_set,   .usage = "OUTPUT INDEX RGB [A]",  .describe = "Set one output pixel to value" },
  { "test",   spi_leds_cmd_test,  .usage = "OUTPUT",                .describe = "Output test patterns" },
  { }
};

const struct cmdtab spi_leds_cmdtab = {
  .commands = spi_leds_commands,
};
