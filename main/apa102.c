#include "apa102.h"
#include <apa102.h>

#include <logging.h>

#define APA102_SPI_HOST HSPI_HOST
#define APA102_SPI_CLK_DIV SPI_2MHz_DIV

struct apa102_config {
  bool enabled;
  uint16_t count;

/* TODO
  uint16_t led_gpio;
  bool artnet_enabled;
  uint16_t artnet_universe;
*/
};

struct apa102 *apa102;
struct apa102_config apa102_config = {

};

const struct configtab apa102_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .value  = { .boolean = &apa102_config.enabled },
  },
  { CONFIG_TYPE_UINT16, "count",
    .value  = { .uint16 = &apa102_config.count },
  },
/*
  { CONFIG_TYPE_UINT16, "led_gpio",
    .value  = { .uint16 = &apa102_config.led_gpio },
  },
  { CONFIG_TYPE_BOOL, "artnet_enabled",
    .value  = { .boolean = &apa102_config.artnet_enabled },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe",
    .value  = { .uint16 = &apa102_config.artnet_universe },
  },
*/
  {}
};

int init_apa102_config(const struct apa102_config *config)
{
  struct apa102_options options = {
      .spi_host = APA102_SPI_HOST,
      .spi_clk_div = APA102_SPI_CLK_DIV,

      .protocol = APA102_PROTOCOL_COMPAT,
      .count = config->count,
  };
  int err;

  if ((err = apa102_new(&apa102, &options))) {
    LOG_ERROR("apa102_new");
    return err;
  }

  return 0;
}

int init_apa102()
{
  int err;

  if (!apa102_config.enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  if ((err = init_apa102_config(&apa102_config))) {
    LOG_ERROR("init_apa102_config");
    return err;
  }

  return 0;
}

int apa102_cmd_clear(int argc, char **argv, void *ctx)
{
  int err;

  if (!apa102) {
    LOG_WARN("disabled");
    return 1;
  }

  if ((err = apa102_set_all(apa102, 0, 0, 0, 0))) {
    LOG_ERROR("apa102_set_all");
    return err;
  }

  if ((err = apa102_tx(apa102))) {
    LOG_ERROR("apa102_tx");
    return err;
  }

  return 0;
}

int apa102_cmd_all(int argc, char **argv, void *ctx)
{
  int rgb, a = 0xff;
  int err;

  if ((err = cmd_arg_int(argc, argv, 1, &rgb)))
    return err;
  if ((argc > 2) && (err = cmd_arg_int(argc, argv, 2, &a)))
    return err;

  uint8_t global = a >> 3;
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >>  8) & 0xFF;
  uint8_t b = (rgb >>  0) & 0xFF;

  if (!apa102) {
    LOG_WARN("disabled");
    return 1;
  }

  if ((err = apa102_set_all(apa102, global, b, g, r))) {
    LOG_ERROR("apa102_set_all");
    return err;
  }

  if ((err = apa102_tx(apa102))) {
    LOG_ERROR("apa102_tx");
    return err;
  }

  return 0;
}

int apa102_cmd_set(int argc, char **argv, void *ctx)
{
  unsigned index;
  int rgb, a = 0xff;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &index)))
    return err;
  if ((err = cmd_arg_int(argc, argv, 2, &rgb)))
    return err;
  if ((argc > 3) && (err = cmd_arg_int(argc, argv, 3, &a)))
    return err;

  uint8_t global = a >> 3;
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >>  8) & 0xFF;
  uint8_t b = (rgb >>  0) & 0xFF;

  if (!apa102) {
    LOG_WARN("disabled");
    return 1;
  }

  if ((err = apa102_set(apa102, index, global, b, g, r))) {
    LOG_ERROR("apa102_set");
    return err;
  }

  if ((err = apa102_tx(apa102))) {
    LOG_ERROR("apa102_tx");
    return err;
  }

  return 0;
}

const struct cmd apa102_commands[] = {
  { "clear",  apa102_cmd_clear, &apa102, .usage = "",               .describe = "Clear values" },
  { "all",    apa102_cmd_all,   &apa102, .usage = "RGB [A]",        .describe = "Set all pixels to values" },
  { "set",    apa102_cmd_set,   &apa102, .usage = "INDEX RGB [A]",  .describe = "Set one pixel to value" },
  { }
};

const struct cmdtab apa102_cmdtab = {
  .commands = apa102_commands,
};
