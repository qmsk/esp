#include "leds.h"
#include "leds_state.h"
#include "leds_artnet.h"
#include "leds_test.h"
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
    printf("\t%-20s: %s\n", "Parameter", config_enum_to_string(leds_parameter_enum, leds_parameter_type(state->leds)));
    printf("\t%-20s: %5u\n", "Count", options->count);
    printf("\t%-20s: %5u\n", "Limit (total)", options->limit_total);
    printf("\t%-20s: %5u / %5u\n", "Limit (group)", options->limit_group, options->limit_groups);

    if (state->artnet) {
      printf("\t%-20s:\n", "Art-Net");
      printf("\t\t%-20s: %5u\n", "Network", config->artnet_net);
      printf("\t\t%-20s: %5u\n", "Sub-Net", config->artnet_subnet);
      printf("\t\t%-20s: %5u\n", "Universe Start", config->artnet_universe_start);
      printf("\t\t%-20s: %5u\n", "Universe Count", state->artnet->universe_count);
      printf("\t\t%-20s: %5u\n", "Universe LEDs", state->artnet->universe_leds_count);
      printf("\t\t%-20s: %5u\n", "LEDS Segment", config->artnet_leds_segment);
      printf("\t\t%-20s: %5u\n", "LEDS Group", config->artnet_leds_group);
      printf("\t\t%-20s: %s\n", "LEDS Format", config_enum_to_string(leds_format_enum, config->artnet_leds_format));
      printf("\t\t%-20s: %u\n", "DMX Timeout", config->artnet_dmx_timeout);
    }
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

    bool active = leds_is_active(state->leds);
    struct leds_limit_status limit_total_status;
    struct leds_limit_status limit_groups_status[LEDS_LIMIT_GROUPS_MAX];
    size_t groups = LEDS_LIMIT_GROUPS_MAX;
    TickType_t tick = xTaskGetTickCount();

    leds_get_limit_total_status(state->leds, &limit_total_status);
    leds_get_limit_groups_status(state->leds, limit_groups_status, &groups);

    printf("\tActive    : %s\n", active ? "true" : "false");
    printf("\tUpdate    : %dms\n", state->update_tick ? (tick - state->update_tick) * portTICK_RATE_MS : 0);
    if (state->test) {
      printf("\tTest:\n");
      printf("\t\tMode : %s\n", state->test->mode ? config_enum_to_string(leds_test_mode_enum, state->test->mode) : "");
    }
    if (state->artnet) {
      printf("\tArt-Net:\n");
      printf("\t\tUpdate    : %dms\n", state->artnet->dmx_tick ? (tick - state->artnet->dmx_tick) * portTICK_RATE_MS : 0);
    }

    printf("\tLimit:\n");
    printf("\t\tTotal    : count %5d power %5.1f%% limit %5.1f%% util %5.1f%% applied %5.1f%% output %5.1f%%\n",
      limit_total_status.count,
      leds_limit_status_power(&limit_total_status) * 100.0f,
      leds_limit_status_limit(&limit_total_status) * 100.0f,
      leds_limit_status_util(&limit_total_status) * 100.0f,
      leds_limit_status_applied(&limit_total_status) * 100.0f,
      leds_limit_status_output(&limit_total_status) * 100.0f
    );

    for (unsigned j = 0; j < groups; j++) {
      printf("\t\tGroup[%2d]: count %5d power %5.1f%% limit %5.1f%% util %5.1f%% applied %5.1f%% output %5.1f%%\n", j,
        limit_groups_status[j].count,
        leds_limit_status_power(&limit_groups_status[j]) * 100.0f,
        leds_limit_status_limit(&limit_groups_status[j]) * 100.0f,
        leds_limit_status_util(&limit_groups_status[j]) * 100.0f,
        leds_limit_status_applied(&limit_groups_status[j]) * 100.0f,
        leds_limit_status_output(&limit_groups_status[j]) * 100.0f
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

    if ((err = update_leds(state, USER_ACTIVITY_LEDS_CMD))) {
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

    switch (leds_parameter_type(state->leds)) {
      case LEDS_PARAMETER_NONE:
        break;

      case LEDS_PARAMETER_DIMMER:
        leds_color.dimmer = a;
        break;

      case LEDS_PARAMETER_WHITE:
        leds_color.white = w;
        break;
    }

    if ((err = leds_set_all(state->leds, leds_color))) {
      LOG_ERROR("leds_set_all");
      return err;
    }

    if ((err = update_leds(state, USER_ACTIVITY_LEDS_CMD))) {
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

  switch (leds_parameter_type(state->leds)) {
    case LEDS_PARAMETER_NONE:
      break;

    case LEDS_PARAMETER_DIMMER:
      leds_color.dimmer = a;
      break;

    case LEDS_PARAMETER_WHITE:
      leds_color.white = w;
      break;
  }

  if ((err = leds_set(state->leds, index, leds_color))) {
    LOG_ERROR("leds_set");
    return err;
  }

  if ((err = update_leds(state, USER_ACTIVITY_LEDS_CMD))) {
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
  const char *mode_arg = NULL;
  enum leds_test_mode mode = 0;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &leds_id)))
    return err;

  if (argc > 2 && (err = cmd_arg_str(argc, argv, 2, &mode_arg)))
    return err;

  if ((err = lookup_leds(leds_id, &config, &state))) {
    return err;
  }

  if (mode_arg && (mode = config_enum_to_value(leds_test_mode_enum, mode_arg)) < 0) {
    LOG_ERROR("invalid mode=%s", mode_arg);
    return -1;
  }

  if (mode) {
    if ((err = test_leds_mode(state, mode))) {
      LOG_ERROR("test_leds_mode");
      return err;
    }
  } else {
    if ((err = test_leds(state))) {
      LOG_ERROR("test_leds");
      return err;
    }
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

    if ((err = update_leds(state, USER_ACTIVITY_LEDS_CMD))) {
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

      if ((err = update_leds(state, USER_ACTIVITY_LEDS_CMD))) {
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

  #if LEDS_I2S_INTERFACE_COUNT > 0
    print_stats_timer("i2s0", "open",    &stats.i2s0.open);
    print_stats_timer("i2s0", "write",   &stats.i2s0.write);
    print_stats_timer("i2s0", "start",   &stats.i2s0.start);
    print_stats_timer("i2s0", "flush",   &stats.i2s0.flush);
    printf("\n");
  #endif
  #if LEDS_I2S_INTERFACE_COUNT > 1
    print_stats_timer("i2s1", "open",    &stats.i2s1.open);
    print_stats_timer("i2s1", "write",   &stats.i2s1.write);
    print_stats_timer("i2s1", "start",   &stats.i2s1.start);
    print_stats_timer("i2s1", "flush",   &stats.i2s1.flush);
    printf("\n");
  #endif
  }

  // sequence
  {
    struct leds_sequence_stats *stats = &leds_sequence_stats;

    print_stats_timer("sequence", "read", &stats->read);
    print_stats_counter("sequence", "skip", &stats->skip);
  }

  for (unsigned i = 0; i < LEDS_COUNT; i++) {
    const struct leds_config *config = &leds_configs[i];
    const struct leds_stats *stats = &leds_stats[i];

    if (!config->enabled) {
      continue;
    }

    printf("leds%u:\n", i + 1);

    print_stats_timer("task", "loop",     &stats->loop);
    print_stats_timer("task", "test",     &stats->test);
    print_stats_timer("task", "artnet",   &stats->artnet);
    print_stats_timer("task", "sequence", &stats->sequence);
    print_stats_timer("task", "update",   &stats->update);
    printf("\n");
    print_stats_counter("artnet", "timeout", &stats->artnet_timeout);
    print_stats_counter("artnet", "sync",    &stats->artnet_sync);
    print_stats_counter("sync",   "none",    &stats->sync_none);
    print_stats_counter("sync",   "timeout", &stats->sync_timeout);
    print_stats_counter("sync",   "missed",  &stats->sync_missed);
    print_stats_counter("sync",   "full",    &stats->sync_full);
    print_stats_counter("update", "timeout", &stats->update_timeout);
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
  { "test",     leds_cmd_test,    .usage = "LEDS-ID [MODE]",            .describe = "Output test patterns" },
  { "stats",    leds_cmd_stats,   .usage = "[reset]",                   .describe = "Show/reset LED stats" },
  { }
};

const struct cmdtab leds_cmdtab = {
  .commands = leds_commands,
};
