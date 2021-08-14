#include "spi_leds.h"
#include "spi_leds_artnet.h"
#include "activity_led.h"
#include "atx_psu.h"

#include <spi_master.h>
#include <spi_leds.h>

#include <logging.h>

#define SPI_LEDS_MODE (SPI_MODE_0) // varies by protocol
#define SPI_LEDS_CLOCK (SPI_CLOCK_1MHZ)
#define SPI_LEDS_PINS (SPI_PINS)

struct spi_leds_config {
  bool enabled;
  int protocol;
  int spi_clock;
  uint16_t count;

  bool artnet_enabled;
  uint16_t artnet_universe;
  int artnet_mode;
};

struct spi_leds_config spi_leds_config = {
  .spi_clock   = SPI_LEDS_CLOCK,
  .protocol    = SPI_LEDS_PROTOCOL_APA102,
  .artnet_mode = SPI_LEDS_BGR,
};

const struct config_enum spi_leds_protocol_enum[] = {
  { "APA102", SPI_LEDS_PROTOCOL_APA102  },
  { "P9813",  SPI_LEDS_PROTOCOL_P9813   },
  {}
};

const struct config_enum spi_leds_rate_enum[] = {
  { "20M",  SPI_CLOCK_20MHZ   },
  { "10M",  SPI_CLOCK_10MHZ   },
  { "5M",   SPI_CLOCK_5MHZ    },
  { "2M",   SPI_CLOCK_2MHZ    },
  { "1M",   SPI_CLOCK_1MHZ    },
  { "500K", SPI_CLOCK_500KHZ  },
  { "200K", SPI_CLOCK_200KHZ  },
  { "100K", SPI_CLOCK_100KHZ  },
  { "50K",  SPI_CLOCK_50KHZ   },
  { "20K",  SPI_CLOCK_20KHZ   },
  { "10K",  SPI_CLOCK_10KHZ   },
  { "1K",   SPI_CLOCK_1KHZ    },
  {}
};

const struct config_enum spi_leds_artnet_mode_enum[] = {
  { "RGB", SPI_LEDS_RGB  },
  { "BGR", SPI_LEDS_BGR  },
  { "GRB", SPI_LEDS_GRB  },
  {}
};

const struct configtab spi_leds_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .value  = { .boolean = &spi_leds_config.enabled },
  },
  { CONFIG_TYPE_ENUM, "protocol",
    .enum_values = spi_leds_protocol_enum,
    .value       = { .enum_value = &spi_leds_config.protocol },
  },
  { CONFIG_TYPE_ENUM, "rate",
    .enum_values = spi_leds_rate_enum,
    .value       = { .enum_value = &spi_leds_config.spi_clock },
  },
  { CONFIG_TYPE_UINT16, "count",
    .value  = { .uint16 = &spi_leds_config.count },
  },
  { CONFIG_TYPE_BOOL, "artnet_enabled",
    .value  = { .boolean = &spi_leds_config.artnet_enabled },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe",
    .value  = { .uint16 = &spi_leds_config.artnet_universe },
  },
  { CONFIG_TYPE_ENUM, "artnet_mode",
    .enum_values = spi_leds_artnet_mode_enum,
    .value       = { .enum_value = &spi_leds_config.artnet_mode },
  },
  {}
};

struct spi_master *spi_master;
struct spi_leds *spi_leds;

bool spi_leds_activated;

int config_spi_master(const struct spi_leds_config *config)
{
  struct spi_options options = {
      .mode   = SPI_LEDS_MODE,
      .clock  = config->spi_clock,
      .pins   = SPI_LEDS_PINS,
  };
  int err;

  LOG_INFO("mode=%02x clock=%u pins=%02x", options.mode, options.clock, options.pins);

  if ((err = spi_master_new(&spi_master, options))) {
    LOG_ERROR("spi_master_new");
    return err;
  }

  return 0;
}

int config_spi_leds(const struct spi_leds_config *config)
{
  struct spi_leds_options options = {
      .protocol = config->protocol,
      .clock    = config->spi_clock,
      .count    = config->count,
  };
  int err;

  LOG_INFO("protocol=%u count=%u", options.protocol, options.count);

  if ((err = spi_leds_new(&spi_leds, spi_master, options))) {
    LOG_ERROR("spi_leds_new");
    return err;
  }

  if (spi_leds_config.artnet_enabled) {
    LOG_INFO("artnet universe=%u mode=%u", spi_leds_config.artnet_universe, spi_leds_config.artnet_mode);

    if ((err = init_spi_leds_artnet(spi_leds_config.artnet_universe, spi_leds_config.artnet_mode))) {
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

  if ((err = config_spi_master(&spi_leds_config))) {
    LOG_ERROR("config_spi_master");
    return err;
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
