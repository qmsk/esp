#include "leds.h"
#include "leds_state.h"
#include "leds_config.h"
#include "leds_stats.h"

#include <logging.h>
#include <leds.h>

int leds_cmd_info(int argc, char **argv, void *ctx)
{
  for (int i = 0; i < LEDS_COUNT; i++) {
    const struct leds_config *config = &leds_configs[i];
    struct leds_state *state = &leds_states[i];

    printf("leds%d:\n", i + 1);
    printf("\t%-20s: %s\n", "Enabled", config->enabled ? "true" : "false");
    printf("\t%-20s: %s\n", "Initialized", state->leds ? "true" : "false");

    if (!config->enabled || !state->leds) {
      continue;
    }

    const struct leds_options *options = leds_options(state->leds);

    printf("\t%-20s: %s\n", "Interface", config_enum_to_string(leds_interface_enum, options->interface));
    printf("\t%-20s: %s\n", "Protocol", config_enum_to_string(leds_protocol_enum, options->protocol));
    printf("\t%-20s: %s\n", "Color Parameter", config_enum_to_string(leds_color_parameter_enum, leds_color_parameter_for_protocol(options->protocol)));
    printf("\t%-20s: %u\n", "Count", options->count);
    printf("\t%-20s: %u\n", "Active", state->active);
    printf("\n");
  }

  return 0;
}

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
  unsigned leds_id, index;
  int rgb, a = 0xff, w = 0;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &leds_id)))
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

  if (leds_id < 1 || leds_id > LEDS_COUNT) {
    LOG_ERROR("leds%u does not exist", leds_id);
    return CMD_ERR_ARGV;
  }

  const struct leds_config *config = &leds_configs[leds_id - 1];
  struct leds_state *state = &leds_states[leds_id - 1];

  if (!config->enabled || !state->leds) {
    LOG_WARN("leds%u is not enabled", leds_id);
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
  unsigned leds_id;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &leds_id)))
    return err;

  if (leds_id < 1 || leds_id > LEDS_COUNT) {
    LOG_ERROR("leds%u does not exist", leds_id);
    return CMD_ERR_ARGV;
  }

  const struct leds_config *config = &leds_configs[leds_id - 1];
  struct leds_state *state = &leds_states[leds_id - 1];

  if (!config->enabled || !state->leds) {
    LOG_WARN("leds%u is not enabled", leds_id);
    return 0;
  }

  if ((err = test_leds(state))) {
    LOG_ERROR("test_leds");
    return err;
  }

  return 0;
}

static void print_stats_timer(const char *title, const char *desc, const struct stats_timer timer)
{
  printf("\t%10s : %-10s %12.3fs total / %6u count @ %12.3fs = %6.1fms avg, %5.1f%% util\n", title, desc,
    (float)(timer.total) / 1000 / 1000,
    timer.count,
    timer.reset < timer.update ? (float)(timer.update - timer.reset) / 1000 / 1000 : 0,
    timer.count ? (float)(timer.total) / (float)(timer.count) / 1000 : 0,
    timer.reset < timer.update ? (float)(timer.total) / (float)(timer.update - timer.reset) * 100 : 0
  );
}

int leds_cmd_stats(int argc, char **argv, void *ctx)
{
  print_stats_timer("artnet", "loop",   leds_stats_artnet_loop);
  print_stats_timer("artnet", "test",   leds_stats_artnet_test);
  print_stats_timer("artnet", "set",    leds_stats_artnet_set);
  print_stats_timer("artnet", "update", leds_stats_artnet_update);

  return 0;
}

const struct cmd leds_commands[] = {
  { "info",   leds_cmd_info,                                        .describe = "Show LED info" },
  { "clear",  leds_cmd_clear,                                       .describe = "Clear all output values" },
  { "all",    leds_cmd_all,   .usage = "RGB [A]",                   .describe = "Set all output pixels to value" },
  { "set",    leds_cmd_set,   .usage = "LEDS-ID LED-INDEX RGB [A]", .describe = "Set one output pixel to value" },
  { "test",   leds_cmd_test,  .usage = "LEDS-ID",                   .describe = "Output test patterns" },
  { "stats",  leds_cmd_stats,                                       .describe = "Show LED stats" },
  { }
};

const struct cmdtab leds_cmdtab = {
  .commands = leds_commands,
};
