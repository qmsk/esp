#include "leds.h"
#include "leds_state.h"
#include "leds_config.h"
#include "leds_stats.h"

#include <logging.h>
#include <leds.h>
#include <leds_stats.h>
#include <leds_status.h>
#include <stats_print.h>

#include <string.h>

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
    printf("\t%-20s: %5u\n", "Count", options->count);
    printf("\t%-20s: %5u\n", "Limit (total)", options->limit_total);
    printf("\t%-20s: %5u / %5u\n", "Limit (group)", options->limit_group, options->limit_groups);
    printf("\n");
  }

  return 0;
}

int leds_cmd_status(int argc, char **argv, void *ctx)
{
  for (int i = 0; i < LEDS_COUNT; i++) {
    const struct leds_config *config = &leds_configs[i];
    struct leds_state *state = &leds_states[i];

    if (!config->enabled || !state->leds) {
      continue;
    }

    printf("leds%d:\n", i + 1);

    struct leds_limit_status limit_total_status;
    struct leds_limit_status limit_groups_status[LEDS_LIMIT_GROUPS_MAX];
    size_t groups = LEDS_LIMIT_GROUPS_MAX;

    leds_get_limit_total_status(state->leds, &limit_total_status);
    leds_get_limit_groups_status(state->leds, limit_groups_status, &groups);

    printf("\tActive   : %5u\n", state->active);
    printf("\tTotal    : config %5.1f%% util %5.1f%% applied %5.1f%%\n",
      leds_limit_status_configured(&limit_total_status) * 100.0f,
      leds_limit_status_utilization(&limit_total_status) * 100.0f,
      leds_limit_status_active(&limit_total_status) * 100.0f
    );

    for (unsigned j = 0; j < groups; j++) {
      printf("\tGroup[%2d]: config %5.1f%% util %5.1f%% applied %5.1f%%\n", j,
        leds_limit_status_configured(&limit_groups_status[j]) * 100.0f,
        leds_limit_status_utilization(&limit_groups_status[j]) * 100.0f,
        leds_limit_status_active(&limit_groups_status[j]) * 100.0f
      );
    }

    printf("\n");
  }

  return 0;
}

static int lookup_leds(unsigned index, const struct leds_config **configp, struct leds_state **statep)
{
  if (index < 1 || index > LEDS_COUNT) {
    LOG_ERROR("leds%u does not exist", index);
    return CMD_ERR_ARGV;
  }

  const struct leds_config *config = &leds_configs[index - 1];
  struct leds_state *state = &leds_states[index - 1];

  if (!config->enabled || !state->leds) {
    LOG_WARN("leds%u is not enabled", index);
    return 1;
  }

  *configp = config;
  *statep = state;

  return 0;
}

int leds_cmd_clear(int argc, char **argv, void *ctx)
{
  int err;

  for (int i = 0; i < LEDS_COUNT; i++) {
    const struct leds_config *config = &leds_configs[i];
    struct leds_state *state = &leds_states[i];

    if (!config->enabled || !state->leds) {
      continue;
    }

    if ((err = leds_clear_all(state->leds))) {
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

  struct leds_color leds_color = {
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
        leds_color.dimmer = a;
        break;

      case LEDS_COLOR_WHITE:
        leds_color.white = w;
        break;
    }

    if ((err = leds_set_all(state->leds, leds_color))) {
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
  const struct leds_config *config;
  struct leds_state *state;
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

  if ((err = lookup_leds(leds_id, &config, &state))) {
    return err;
  }

  struct leds_color leds_color = {
    .r = (rgb >> 16) & 0xFF,
    .g = (rgb >>  8) & 0xFF,
    .b = (rgb >>  0) & 0xFF,
  };

  switch (leds_color_parameter_for_protocol(config->protocol)) {
    case LEDS_COLOR_NONE:
      break;

    case LEDS_COLOR_DIMMER:
      leds_color.dimmer = a;
      break;

    case LEDS_COLOR_WHITE:
      leds_color.white = w;
      break;
  }

  if ((err = leds_set(state->leds, index, leds_color))) {
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
  const struct leds_config *config;
  struct leds_state *state;
  unsigned leds_id;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &leds_id)))
    return err;

  if ((err = lookup_leds(leds_id, &config, &state))) {
    return err;
  }

  if ((err = test_leds(state))) {
    LOG_ERROR("test_leds");
    return err;
  }

  return 0;
}

int leds_cmd_update(int argc, char **argv, void *ctx)
{
  const struct leds_config *config;
  struct leds_state *state;
  unsigned leds_id = 0;
  int err;

  if (argc > 1 && (err = cmd_arg_uint(argc, argv, 1, &leds_id)))
    return err;

  if (leds_id) {
    if ((err = lookup_leds(leds_id, &config, &state))) {
      return err;
    }

    if ((err = update_leds(state))) {
      LOG_ERROR("update_leds");
      return err;
    }
  } else {
    for (int i = 0; i < LEDS_COUNT; i++) {
      const struct leds_config *config = &leds_configs[i];
      struct leds_state *state = &leds_states[i];

      if (!config->enabled || !state->leds) {
        continue;
      }

      if ((err = update_leds(state))) {
        LOG_ERROR("update_leds");
        return err;
      }
    }
  }

  return 0;
}

int leds_cmd_stats(int argc, char **argv, void *ctx)
{
  // interface
  {
    struct leds_interface_stats stats;

    leds_get_interface_stats(&stats);

  #if CONFIG_LEDS_SPI_ENABLED
    print_stats_timer("spi", "open",   &stats.spi.open);
    print_stats_timer("spi", "tx",     &stats.spi.tx);
    printf("\n");
  #endif

  #if CONFIG_LEDS_UART_ENABLED
    print_stats_timer("uart", "open",   &stats.uart.open);
    print_stats_timer("uart", "tx",     &stats.uart.tx);
    printf("\n");
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    print_stats_timer("i2s", "open",    &stats.i2s.open);
    print_stats_timer("i2s", "write",   &stats.i2s.write);
    print_stats_timer("i2s", "flush",   &stats.i2s.flush);
    printf("\n");
  #endif
  }

  for (unsigned i = 0; i < LEDS_COUNT; i++) {
    const struct leds_artnet_stats *stats = &leds_artnet_stats[i];

    printf("leds%u:\n", i + 1);

    print_stats_timer("artnet", "loop",   &stats->loop);
    print_stats_timer("artnet", "test",   &stats->test);
    print_stats_timer("artnet", "set",    &stats->set);
    print_stats_timer("artnet", "update", &stats->update);
    printf("\n");
    print_stats_counter("artnet", "timeout", &stats->timeout);
    printf("\n");
  }

  if (argc > 1 && strcmp(argv[1], "reset") == 0) {
    LOG_INFO("reset leds stats");

    init_leds_stats();
    leds_reset_interface_stats();
  }

  return 0;
}

const struct cmd leds_commands[] = {
  { "info",     leds_cmd_info,                                          .describe = "Show LED info" },
  { "status",   leds_cmd_status,                                        .describe = "Show LED status" },
  { "clear",    leds_cmd_clear,                                         .describe = "Clear all output values" },
  { "all",      leds_cmd_all,     .usage = "RGB [A]",                   .describe = "Set all output pixels to value" },
  { "set",      leds_cmd_set,     .usage = "LEDS-ID LED-INDEX RGB [A]", .describe = "Set one output pixel to value" },
  { "update",   leds_cmd_update,  .usage = "[LEDS-ID]",                 .describe = "Refresh one or all LED outputs" },
  { "test",     leds_cmd_test,    .usage = "LEDS-ID",                   .describe = "Output test patterns" },
  { "stats",    leds_cmd_stats,   .usage = "[reset]",                   .describe = "Show/reset LED stats" },
  { }
};

const struct cmdtab leds_cmdtab = {
  .commands = leds_commands,
};
