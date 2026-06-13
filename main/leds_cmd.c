#include "leds.h"
#include "leds_state.h"
#include "leds_static.h"
#include "leds_status.h"
#include "leds_artnet.h"
#include "leds_task.h"
#include "leds_test.h"
#include "leds_config.h"
#include "leds_stats.h"

#include <logging.h>
#include <leds.h>
#include <leds_stats.h>
#include <leds_status.h>
#include <stats_print.h>

#include <string.h>

#define CMD_LEDS_MUTEX_TIMEOUT (1000 / portTICK_RATE_MS)

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
  struct leds_status status;

  for (int i = 0; i < LEDS_COUNT; i++) {
    struct leds_state *state = &leds_states[i];

    if (!state->leds) {
      continue;
    }

    get_leds_status(&leds_states[i], &status);

    printf("leds%d:\n", i + 1);

    printf("\tActive    : %s\n", status.active ? "true" : "false");
    printf("\tUpdate    : %dms\n", status.update_tick ? (status.tick - status.update_tick) * portTICK_RATE_MS : 0);
    printf("\tTask      : %6.1f/s @ %5.1f%% (%.0fs)\n", status.metrics.task.rate, status.metrics.task.util * 100.0f, status.metrics.task.interval);
    printf("\tInterface : %6.1f/s @ %5.1f%% (%.0fs)\n", status.metrics.interface.rate, status.metrics.interface.util * 100.0f, status.metrics.interface.interval);
    if (status.test) {
      printf("\tTest:\n");
      printf("\t\tMode : %s\n", status.test_mode ? config_enum_to_string(leds_test_mode_enum, status.test_mode) : "");
    }
    if (status.artnet) {
      printf("\tArt-Net:\n");
      printf("\t\tUpdate    : %dms\n", status.artnet_dmx_tick ? (status.tick - status.artnet_dmx_tick) * portTICK_RATE_MS : 0);
    }

    printf("\tLimit:\n");
    printf("\t\tTotal    : count %5d power %5.1f%% limit %5.1f%% util %5.1f%% applied %5.1f%% output %5.1f%%\n",
      status.limit_total_status.count,
      leds_limit_status_power(&status.limit_total_status) * 100.0f,
      leds_limit_status_limit(&status.limit_total_status) * 100.0f,
      leds_limit_status_util(&status.limit_total_status) * 100.0f,
      leds_limit_status_applied(&status.limit_total_status) * 100.0f,
      leds_limit_status_output(&status.limit_total_status) * 100.0f
    );

    for (unsigned j = 0; j < status.limit_groups_count; j++) {
      printf("\t\tGroup[%2d]: count %5d power %5.1f%% limit %5.1f%% util %5.1f%% applied %5.1f%% output %5.1f%%\n", j,
        status.limit_groups_status[j].count,
        leds_limit_status_power(&status.limit_groups_status[j]) * 100.0f,
        leds_limit_status_limit(&status.limit_groups_status[j]) * 100.0f,
        leds_limit_status_util(&status.limit_groups_status[j]) * 100.0f,
        leds_limit_status_applied(&status.limit_groups_status[j]) * 100.0f,
        leds_limit_status_output(&status.limit_groups_status[j]) * 100.0f
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

    if ((err = start_leds_update(state, LEDS_UPDATE_CMD))) {
      LOG_ERROR("start_leds_update");
      continue;
    }

    leds_clear_all(state->leds);

    end_leds_update(state);
  }

  return 0;
}

int leds_cmd_static(int argc, char **argv, void *ctx)
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

    if ((err = set_leds_static(state, leds_color))) {
      LOG_ERROR("set_leds_static");
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

    if ((err = start_leds_update(state, LEDS_UPDATE_CMD))) {
      LOG_ERROR("start_leds_update");
      continue;
    }

    leds_set_all(state->leds, leds_color);

    end_leds_update(state);
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

  if ((err = start_leds_update(state, LEDS_UPDATE_CMD))) {
    LOG_ERROR("start_leds_update");
    return -1;
  }

  if ((err = leds_set(state->leds, index, leds_color))) {
    LOG_WARN("leds_set");
    goto error;
  }

error:
  end_leds_update(state);

  return err;
}

int leds_cmd_test(int argc, char **argv, void *ctx)
{
  const char *mode_arg = NULL;
  enum leds_test_mode mode = 0;
  int err;

  if (argc == 1){

  } else if ((err = cmd_arg_str(argc, argv, 1, &mode_arg))) {
    return err;
  } else if ((mode = config_enum_to_value(leds_test_mode_enum, mode_arg)) < 0) {
    LOG_ERROR("invalid mode=%s", mode_arg);
    return -1;
  }

  for (int i = 0; i < LEDS_COUNT; i++) {
    struct leds_state *state = &leds_states[i];

    if (!state->leds || !state->test) {
      continue;
    }

    if (mode) {
      if ((err = set_leds_test(state, mode, false))) {
        LOG_ERROR("set_leds_test");
        return err;
      }
    } else {
      if ((err = set_leds_test_next(state))) {
        LOG_ERROR("set_leds_test_next");
        return err;
      }
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

    if ((err = start_leds_update(state, LEDS_UPDATE_CMD))) {
      LOG_ERROR("start_leds_update");
      return err;
    }

    end_leds_update(state);
  } else {
    for (int i = 0; i < LEDS_COUNT; i++) {
      const struct leds_config *config = &leds_configs[i];
      struct leds_state *state = &leds_states[i];

      if (!config->enabled || !state->leds) {
        continue;
      }

      if ((err = start_leds_update(state, LEDS_UPDATE_CMD))) {
        LOG_ERROR("start_leds_update");
        return err;
      }

      end_leds_update(state);
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
    struct i2s_out_stats i2s0_stats = get_leds_i2s_out_stats(0);

    print_stats_timer("i2s0", "out",     &i2s0_stats.out_timer);
    print_stats_timer("i2s0", "open",    &stats.i2s0.open);
    print_stats_timer("i2s0", "write",   &stats.i2s0.write);
    print_stats_timer("i2s0", "start",   &stats.i2s0.start);
    print_stats_timer("i2s0", "flush",   &stats.i2s0.flush);
    printf("\n");
  #endif
  #if LEDS_I2S_INTERFACE_COUNT > 1
    struct i2s_out_stats i2s1_stats = get_leds_i2s_out_stats(1);

    print_stats_timer("i2s1", "out",     &i2s1_stats.out_timer);
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

    print_stats_timer("update", "",       &stats->update);
    print_stats_timer("task", "loop",     &stats->loop);
    print_stats_timer("task", "test",     &stats->test);
    print_stats_timer("task", "artnet",   &stats->artnet);
    print_stats_timer("task", "sequence", &stats->sequence);
    print_stats_timer("task", "static",   &stats->static_);
    print_stats_timer("task", "output",   &stats->output);
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
    reset_leds_i2s_out_stats();
    leds_reset_interface_stats();
  }

  return 0;
}

const struct cmd leds_commands[] = {
  { "info",     leds_cmd_info,                                          .describe = "Show LED info" },
  { "status",   leds_cmd_status,                                        .describe = "Show LED status" },
  { "clear",    leds_cmd_clear,                                         .describe = "Clear test patterns" },
  { "static",   leds_cmd_static,  .usage = "RGB [A]",                   .describe = "Set static LEDs color" },
  { "all",      leds_cmd_all,     .usage = "RGB [A]",                   .describe = "Set all output pixels to value" },
  { "set",      leds_cmd_set,     .usage = "LEDS-ID LED-INDEX RGB [A]", .describe = "Set one output pixel to value" },
  { "update",   leds_cmd_update,  .usage = "[LEDS-ID]",                 .describe = "Refresh one or all LED outputs" },
  { "test",     leds_cmd_test,    .usage = "[MODE]",                    .describe = "Output test patterns" },
  { "stats",    leds_cmd_stats,   .usage = "[reset]",                   .describe = "Show/reset LED stats" },
  { }
};

const struct cmdtab leds_cmdtab = {
  .commands = leds_commands,
};
