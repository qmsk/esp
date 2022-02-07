#include "leds.h"

#include <logging.h>
#include <leds.h>

int leds_cmd_clear(int argc, char **argv, void *ctx)
{
  struct spi_led_color spi_led_color = { }; // off
  int err;

  for (int i = 0; i < LEDS_COUNT; i++) {
    const struct leds_config *config = &leds_configs[i];
    struct leds_state *state = &leds_states[i];

    if (!config->enabled || !state->leds) {
      continue;
    }

    if ((err = leds_set_all(state->leds, spi_led_color))) {
      LOG_ERROR("leds_set_all");
      return err;
    }

    if ((err = update_leds(state))) {
      LOG_ERROR("update_leds");
      return err;
    }
  }

  return 0;
}

int leds_cmd_all(int argc, char **argv, void *ctx)
{
  int rgb, a = 0xff, w = 0;
  int err;

  if ((err = cmd_arg_int(argc, argv, 1, &rgb)))
    return err;
  if ((argc > 2) && (err = cmd_arg_int(argc, argv, 2, &a)))
    return err;
  if ((argc > 2) && (err = cmd_arg_int(argc, argv, 2, &w)))
    return err;

  struct spi_led_color spi_led_color = {
    .r = (rgb >> 16) & 0xFF,
    .g = (rgb >>  8) & 0xFF,
    .b = (rgb >>  0) & 0xFF,
  };

  for (int i = 0; i < LEDS_COUNT; i++) {
    const struct leds_config *config = &leds_configs[i];
    struct leds_state *state = &leds_states[i];

    if (!config->enabled || !state->leds) {
      continue;
    }

    switch (leds_color_parameter_for_protocol(config->protocol)) {
      case LEDS_COLOR_NONE:
        break;

      case LEDS_COLOR_DIMMER:
        spi_led_color.dimmer = a;
        break;

      case LEDS_COLOR_WHITE:
        spi_led_color.white = w;
        break;
    }

    if ((err = leds_set_all(state->leds, spi_led_color))) {
      LOG_ERROR("leds_set_all");
      return err;
    }

    if ((err = update_leds(state))) {
      LOG_ERROR("update_leds");
      return err;
    }
  }

  return 0;
}

int leds_cmd_set(int argc, char **argv, void *ctx)
{
  unsigned output, index;
  int rgb, a = 0xff, w = 0;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &output)))
    return err;
  if ((err = cmd_arg_uint(argc, argv, 2, &index)))
    return err;
  if ((err = cmd_arg_int(argc, argv, 3, &rgb)))
    return err;
  if ((argc > 4) && (err = cmd_arg_int(argc, argv, 4, &a)))
    return err;
  if ((argc > 4) && (err = cmd_arg_int(argc, argv, 4, &w)))
    return err;

  struct spi_led_color spi_led_color = {
    .r = (rgb >> 16) & 0xFF,
    .g = (rgb >>  8) & 0xFF,
    .b = (rgb >>  0) & 0xFF,
  };

  if (output >= LEDS_COUNT) {
    LOG_ERROR("output=%u does not exist", output);
    return CMD_ERR_ARGV;
  }

  const struct leds_config *config = &leds_configs[output];
  struct leds_state *state = &leds_states[output];

  if (!config->enabled || !state->leds) {
    LOG_WARN("output=%u is not enabled", output);
    return 0;
  }

  switch (leds_color_parameter_for_protocol(config->protocol)) {
    case LEDS_COLOR_NONE:
      break;

    case LEDS_COLOR_DIMMER:
      spi_led_color.dimmer = a;
      break;

    case LEDS_COLOR_WHITE:
      spi_led_color.white = w;
      break;
  }

  if ((err = leds_set(state->leds, index, spi_led_color))) {
    LOG_ERROR("leds_set");
    return err;
  }

  if ((err = update_leds(state))) {
    LOG_ERROR("update_leds");
    return err;
  }

  return 0;
}

int leds_cmd_test(int argc, char **argv, void *ctx)
{
  unsigned output;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &output)))
    return err;

  if (output >= LEDS_COUNT) {
    LOG_ERROR("output=%u does not exist", output);
    return CMD_ERR_ARGV;
  }

  const struct leds_config *config = &leds_configs[output];
  struct leds_state *state = &leds_states[output];

  if (!config->enabled || !state->leds) {
    LOG_WARN("output=%u is not enabled", output);
    return 0;
  }

  for (enum leds_test_mode mode = 0; mode <= TEST_MODE_END; mode++) {
    if ((err = test_leds(state, mode))) {
      LOG_ERROR("test_leds");
      return err;
    }
  }

  return 0;
}

const struct cmd leds_commands[] = {
  { "clear",  leds_cmd_clear, .usage = "",                      .describe = "Clear all output values" },
  { "all",    leds_cmd_all,   .usage = "RGB [A]",               .describe = "Set all output pixels to value" },
  { "set",    leds_cmd_set,   .usage = "OUTPUT INDEX RGB [A]",  .describe = "Set one output pixel to value" },
  { "test",   leds_cmd_test,  .usage = "OUTPUT",                .describe = "Output test patterns" },
  { }
};

const struct cmdtab leds_cmdtab = {
  .commands = leds_commands,
};
