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

  bool artnet_enabled;
  uint16_t artnet_universe;
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
  { CONFIG_TYPE_BOOL, "artnet_enabled",
    .value  = { .boolean = &spi_leds_config.artnet_enabled },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe",
    .value  = { .uint16 = &spi_leds_config.artnet_universe },
  },
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

  if (spi_leds_config.artnet_enabled) {
    if ((err = init_spi_leds_artnet(spi_leds_config.artnet_universe))) {
      LOG_ERROR("init_spi_leds_artnet");
      return err;
    }
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

int update_spi_leds()
{
  unsigned active = spi_leds_active(spi_leds);
  int err;

  LOG_DEBUG("active=%u", active);

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
