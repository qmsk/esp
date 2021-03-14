#include "spi_leds.h"
#include "activity_led.h"
#include "atx_psu.h"

#include <spi_leds.h>

#include <logging.h>

#define SPI_LEDS_SPI_HOST HSPI_HOST
#define SPI_LEDS_SPI_CLK_DIV SPI_2MHz_DIV

struct spi_leds_config {
  bool enabled;
  uint16_t count;
  int protocol;

/* TODO
  bool artnet_enabled;
  uint16_t artnet_universe;
*/
};

struct spi_leds *spi_leds;
bool spi_leds_activated;
struct spi_leds_config spi_leds_config = {
  .protocol = SPI_LEDS_PROTOCOL_APA102,
};

const struct config_enum spi_leds_protocol_enum[] = {
  { "APA102", SPI_LEDS_PROTOCOL_APA102  },
  { "P9813",  SPI_LEDS_PROTOCOL_P9813   },
  {}
};

const struct configtab spi_leds_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .value  = { .boolean = &spi_leds_config.enabled },
  },
  { CONFIG_TYPE_UINT16, "count",
    .value  = { .uint16 = &spi_leds_config.count },
  },
  { CONFIG_TYPE_ENUM, "protocol",
    .enum_values = spi_leds_protocol_enum,
    .value       = { .enum_value = &spi_leds_config.protocol },
  },
/*
  { CONFIG_TYPE_BOOL, "artnet_enabled",
    .value  = { .boolean = &spi_leds_config.artnet_enabled },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe",
    .value  = { .uint16 = &spi_leds_config.artnet_universe },
  },
*/
  {}
};

int config_spi_leds(const struct spi_leds_config *config)
{
  struct spi_leds_options options = {
      .spi_host = SPI_LEDS_SPI_HOST,
      .spi_clk_div = SPI_LEDS_SPI_CLK_DIV,

      .protocol = config->protocol,
      .count    = config->count,
  };
  int err;

  if ((err = spi_leds_new(&spi_leds, &options))) {
    LOG_ERROR("spi_leds_new");
    return err;
  }

  return 0;
}

int init_spi_leds()
{
  int err;

  if (!spi_leds_config.enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  if ((err = config_spi_leds(&spi_leds_config))) {
    LOG_ERROR("config_spi_leds");
    return err;
  }

  return 0;
}

static void activate_spi_leds()
{
  LOG_INFO("active");

  activate_atx_psu(ATX_PSU_BIT_SPI_LED);

  spi_leds_activated = true;
}

static void deactivate_spi_leds()
{
  LOG_INFO("inactive");

  deactivate_atx_psu(ATX_PSU_BIT_SPI_LED);

  spi_leds_activated = false;
}

static int update_spi_leds()
{
  unsigned active = spi_leds_active(spi_leds);
  int err;

  LOG_INFO("active=%u", active);

  if (active) {
    if (!spi_leds_activated) {
      activate_spi_leds();
    }
  } else {
    if (spi_leds_activated) {
      deactivate_spi_leds();
    }
  }

  activity_led_event();

  if ((err = spi_leds_tx(spi_leds))) {
    LOG_ERROR("spi_leds_tx");
    return err;
  }

  return 0;
}

int spi_leds_cmd_clear(int argc, char **argv, void *ctx)
{
  int err;

  if (!spi_leds) {
    LOG_WARN("disabled");
    return 1;
  }

  struct spi_led_color spi_led_color = { }; // off

  if ((err = spi_leds_set_all(spi_leds, spi_led_color))) {
    LOG_ERROR("spi_leds_set_all");
    return err;
  }

  if ((err = update_spi_leds())) {
    LOG_ERROR("update_spi_leds");
    return err;
  }

  return 0;
}

int spi_leds_cmd_all(int argc, char **argv, void *ctx)
{
  int rgb, a = 0xff;
  int err;

  if (!spi_leds) {
    LOG_WARN("disabled");
    return 1;
  }

  if ((err = cmd_arg_int(argc, argv, 1, &rgb)))
    return err;
  if ((argc > 2) && (err = cmd_arg_int(argc, argv, 2, &a)))
    return err;

  struct spi_led_color spi_led_color = {
    .r = (rgb >> 16) & 0xFF,
    .g = (rgb >>  8) & 0xFF,
    .b = (rgb >>  0) & 0xFF,

    .parameters.brightness = a,
  };

  if ((err = spi_leds_set_all(spi_leds, spi_led_color))) {
    LOG_ERROR("spi_leds_set_all");
    return err;
  }

  if ((err = update_spi_leds())) {
    LOG_ERROR("update_spi_leds");
    return err;
  }

  return 0;
}

int spi_leds_cmd_set(int argc, char **argv, void *ctx)
{
  unsigned index;
  int rgb, a = 0xff;
  int err;

  if (!spi_leds) {
    LOG_WARN("disabled");
    return 1;
  }

  if ((err = cmd_arg_uint(argc, argv, 1, &index)))
    return err;
  if ((err = cmd_arg_int(argc, argv, 2, &rgb)))
    return err;
  if ((argc > 3) && (err = cmd_arg_int(argc, argv, 3, &a)))
    return err;

  struct spi_led_color spi_led_color = {
    .r = (rgb >> 16) & 0xFF,
    .g = (rgb >>  8) & 0xFF,
    .b = (rgb >>  0) & 0xFF,

    .parameters.brightness = a,
  };

  if ((err = spi_leds_set(spi_leds, index, spi_led_color))) {
    LOG_ERROR("spi_leds_set");
    return err;
  }

  if ((err = update_spi_leds())) {
    LOG_ERROR("update_spi_leds");
    return err;
  }

  return 0;
}

const struct cmd spi_leds_commands[] = {
  { "clear",  spi_leds_cmd_clear, &spi_leds, .usage = "",               .describe = "Clear values" },
  { "all",    spi_leds_cmd_all,   &spi_leds, .usage = "RGB [A]",        .describe = "Set all pixels to values" },
  { "set",    spi_leds_cmd_set,   &spi_leds, .usage = "INDEX RGB [A]",  .describe = "Set one pixel to value" },
  { }
};

const struct cmdtab spi_leds_cmdtab = {
  .commands = spi_leds_commands,
};
