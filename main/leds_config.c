#include "leds.h"
#include "leds_config.h"
#include "leds_state.h"
#include "gpio_type.h"

#include <artnet.h>
#include <leds.h>

#include <logging.h>

#include <sdkconfig.h>

// approximation of how many LEDs will fit per ART-NET Universe
#define LEDS_ARTNET_UNIVERSE_LEDS (ARTNET_DMX_SIZE / 3)

struct leds_config leds_configs[LEDS_COUNT] = {};

const struct config_enum leds_interface_enum[] = {
  { "DEFAULT",  .value = LEDS_INTERFACE_NONE   },
#if CONFIG_LEDS_SPI_ENABLED
  { "SPI",      .value = LEDS_INTERFACE_SPI   },
#endif
#if CONFIG_LEDS_UART_ENABLED
  { "UART",     .value = LEDS_INTERFACE_UART   },
#endif
#if CONFIG_LEDS_I2S_ENABLED
# if LEDS_I2S_INTERFACE_COUNT > 1
  { "I2S0",     .value = LEDS_INTERFACE_I2S0   },
  { "I2S1",     .value = LEDS_INTERFACE_I2S1,   .alias = "I2S"  },
# elif LEDS_I2S_INTERFACE_COUNT > 0
  { "I2S0",     .value = LEDS_INTERFACE_I2S0,   .alias = "I2S"  },
# endif
#endif
  {}
};

const struct config_enum leds_protocol_enum[] = {
  { "NONE",         .value = LEDS_PROTOCOL_NONE        },
  { "APA102",       .value = LEDS_PROTOCOL_APA102      },
  { "P9813",        .value = LEDS_PROTOCOL_P9813       },
  { "WS2812B_GRB",  .value = LEDS_PROTOCOL_WS2812B_GRB,   .alias = "WS2812B"  },
  { "WS2812B_RGB",  .value = LEDS_PROTOCOL_WS2812B_RGB },
  { "SK6812_GRBW",  .value = LEDS_PROTOCOL_SK6812_GRBW },
  { "WS2811_RGB",   .value = LEDS_PROTOCOL_WS2811_RGB,    .alias = "WS2811"   },
  { "WS2811_GRB",   .value = LEDS_PROTOCOL_WS2811_GRB  },
  { "SK9822",       .value = LEDS_PROTOCOL_SK9822      },
  { "SM16703",      .value = LEDS_PROTOCOL_SM16703     },
  {}
};

#if CONFIG_LEDS_SPI_ENABLED && !CONFIG_IDF_TARGET_ESP8266
const struct config_enum leds_spi_cs_mode_enum[] = {
  { "",     .value = LEDS_SPI_CS_MODE_DISABLED },
  { "HIGH", .value = LEDS_SPI_CS_MODE_HIGH     },
  { "LOW",  .value = LEDS_SPI_CS_MODE_LOW      },
  {}
};
#endif

#if CONFIG_LEDS_SPI_ENABLED
const struct config_enum leds_spi_clock_enum[] = {
  { "20M",  .value = SPI_CLOCK_20MHZ   },
  { "10M",  .value = SPI_CLOCK_10MHZ   },
  { "5M",   .value = SPI_CLOCK_5MHZ    },
  { "2M",   .value = SPI_CLOCK_2MHZ    },
  { "1M",   .value = SPI_CLOCK_1MHZ    },
  { "500K", .value = SPI_CLOCK_500KHZ  },
  { "200K", .value = SPI_CLOCK_200KHZ  },
  { "100K", .value = SPI_CLOCK_100KHZ  },
  { "50K",  .value = SPI_CLOCK_50KHZ   },
  { "20K",  .value = SPI_CLOCK_20KHZ   },
  { "10K",  .value = SPI_CLOCK_10KHZ   },
  { "1K",   .value = SPI_CLOCK_1KHZ    },
  {}
};
#endif

#if CONFIG_LEDS_I2S_ENABLED
const struct config_enum leds_i2s_clock_enum[] = {
  { "20M",  .value = I2S_CLOCK_20MHZ   },
  { "10M",  .value = I2S_CLOCK_10MHZ   },
  { "5M",   .value = I2S_CLOCK_5MHZ    },
  { "2M",   .value = I2S_CLOCK_2MHZ    },
  { "1M",   .value = I2S_CLOCK_1MHZ    },
  { "500K", .value = I2S_CLOCK_500KHZ  },
  { "200K", .value = I2S_CLOCK_200KHZ  },
  { "100K", .value = I2S_CLOCK_100KHZ  },
  { "50K",  .value = I2S_CLOCK_50KHZ   },
  {}
};
#endif

#if CONFIG_LEDS_GPIO_ENABLED
const struct config_enum leds_gpio_mode_enum[] = {
  { "",                               .value = LEDS_CONFIG_GPIO_MODE_DISABLED    },
  { "SETUP_LOW",                      .value = LEDS_CONFIG_GPIO_MODE_SETUP_LOW   },
  { "SETUP_HIGH",                     .value = LEDS_CONFIG_GPIO_MODE_SETUP_HIGH  },
  { "ACTIVE_LOW",   .alias = "LOW",   .value = LEDS_CONFIG_GPIO_MODE_ACTIVE_LOW, },
  { "ACTIVE_HIGH",  .alias = "HIGH",  .value = LEDS_CONFIG_GPIO_MODE_ACTIVE_HIGH },
  {}
};
#endif

const struct config_enum leds_format_enum[] = {
  { "RGB",    .value = LEDS_FORMAT_RGB     },
  { "BGR",    .value = LEDS_FORMAT_BGR     },
  { "GRB",    .value = LEDS_FORMAT_GRB     },
  { "RGBA",   .value = LEDS_FORMAT_RGBA    },
  { "RGBW",   .value = LEDS_FORMAT_RGBW    },
  { "RGBxI",  .value = LEDS_FORMAT_RGBXI   },
  { "BGRxI",  .value = LEDS_FORMAT_BGRXI   },
  { "GRBxI",  .value = LEDS_FORMAT_GRBXI   },
  { "RGBWxI", .value = LEDS_FORMAT_RGBWXI  },
  {}
};

const struct config_enum leds_test_mode_enum[] = {
  { "NONE",           .value = TEST_MODE_NONE          },
  { "CHASE",          .value = TEST_MODE_CHASE         },
  { "BLACK_RED",      .value = TEST_MODE_BLACK_RED     },
  { "RED_YELLOW",     .value = TEST_MODE_RED_YELLOW    },
  { "YELLOW_GREEN",   .value = TEST_MODE_YELLOW_GREEN  },
  { "GREEN_CYAN",     .value = TEST_MODE_GREEN_CYAN    },
  { "CYAN_BLUE",      .value = TEST_MODE_CYAN_BLUE     },
  { "BLUE_MAGENTA",   .value = TEST_MODE_BLUE_MAGENTA  },
  { "MAGENTA_RED",    .value = TEST_MODE_MAGENTA_RED   },
  { "RED_BLACK",      .value = TEST_MODE_RED_BLACK     },
  { "BLACK_WHITE",    .value = TEST_MODE_BLACK_WHITE   },
  { "WHITE_RGBW",     .value = TEST_MODE_WHITE_RGBW    },
  { "RGBW_RGB",       .value = TEST_MODE_RGBW_RGB      },
  { "RGB_BLACK",      .value = TEST_MODE_RGB_BLACK     },
  { "RAINBOW",        .value = TEST_MODE_RAINBOW       },
  { "BLACK",          .value = TEST_MODE_BLACK         },
  {}
};

const struct config_enum leds_parameter_enum[] = {
  { "NONE",     .value = LEDS_PARAMETER_NONE   },
  { "DIMMER",   .value = LEDS_PARAMETER_DIMMER },
  { "WHITE",    .value = LEDS_PARAMETER_WHITE  },
  {}
};

#if CONFIG_LEDS_I2S_ENABLED
  static int validate_leds_i2s_parallel (config_invalid_handler_t *handler, const struct config_path path, void *ctx)
  {
    struct leds_config *config = path.tab->ctx;
    unsigned data_width = config_leds_i2s_data_width(config);

    if (config->enabled && data_width > 1) {
      switch (config->interface) {
      #if CONFIG_IDF_TARGET_ESP32
        case LEDS_INTERFACE_I2S1:
          break; // ok
      #endif
        
        default:
          handler(path, ctx, "LEDs interface %s does not support parallel output",
            config_enum_to_string(leds_interface_enum, config->interface)
          );

          return 1;
      }
    }

    return 0;
  }
#endif

static int validate_artnet_leds_group (config_invalid_handler_t *handler, const struct config_path path, void *ctx)
{
  struct leds_config *config = path.tab->ctx;

  // check that groups fits into one Art-NET universe with the given used format
  unsigned universe_leds_count = leds_format_count(ARTNET_DMX_SIZE, config->artnet_leds_format, config->artnet_leds_group);

  if (universe_leds_count == 0) {
    handler(path, ctx, "LEDs group does not fit into one Art-NET universe with format %s",
      config_enum_to_string(leds_format_enum, config->artnet_leds_format)
    );

    return 1;
  }

  return 0;
}

static int validate_artnet_dmx_leds (config_invalid_handler_t *handler, const struct config_path path, void *ctx)
{
  struct leds_config *config = path.tab->ctx;

  // maximum number of pixels that fit into one Art-Net universe
  unsigned universe_leds_count = leds_format_count(ARTNET_DMX_SIZE, config->artnet_leds_format, config->artnet_leds_group);

  if (!config->artnet_dmx_leds) {
    // auto, per artnet_leds_group
    return 0;

  } else if (config->artnet_dmx_leds > universe_leds_count) {
    handler(path, ctx, "LEDs do not fit into one Art-NET universe with format %s",
      config_enum_to_string(leds_format_enum, config->artnet_leds_format)
    );

    return 1;
  }

  return 0;
}

static int validate_artnet_universe_count (config_invalid_handler_t *handler, const struct config_path path, void *ctx)
{
  struct leds_config *config = path.tab->ctx;

  unsigned artnet_universe_count = config_leds_artnet_universe_count(config);

  if (artnet_universe_count > ARTNET_OUTPUTS_MAX) {
    handler(path, ctx, "Art-Net universe count %d exceeds maximum of %d",
      artnet_universe_count,
      ARTNET_OUTPUTS_MAX
    );

    return 1;
  }

  return 0;
}

#define LEDS_CONFIGTAB leds_configtab0
#define LEDS_CONFIG leds_configs[0]
#include "leds_configtab.i"
#undef LEDS_CONFIGTAB
#undef LEDS_CONFIG

#define LEDS_CONFIGTAB leds_configtab1
#define LEDS_CONFIG leds_configs[1]
#include "leds_configtab.i"
#undef LEDS_CONFIGTAB
#undef LEDS_CONFIG

#define LEDS_CONFIGTAB leds_configtab2
#define LEDS_CONFIG leds_configs[2]
#include "leds_configtab.i"
#undef LEDS_CONFIGTAB
#undef LEDS_CONFIG

#define LEDS_CONFIGTAB leds_configtab3
#define LEDS_CONFIG leds_configs[3]
#include "leds_configtab.i"
#undef LEDS_CONFIGTAB
#undef LEDS_CONFIG

const struct configtab *leds_configtabs[LEDS_COUNT] = {
  leds_configtab0,
  leds_configtab1,
  leds_configtab2,
  leds_configtab3,
};

int config_leds(struct leds_state *state, const struct leds_config *config)
{
  struct leds_options options = {
      .interface    = config->interface,
      .protocol     = config->protocol,
      .count        = config->count,
      .limit_total  = config->limit_total,
      .limit_group  = config->limit_group,
      .limit_groups = config->limit_groups,
  };
  int err;

  if (!config->interface) {
    options.interface = leds_interface_for_protocol(options.protocol);
  }

  LOG_INFO("leds%d: interface=%s protocol=%s count=%u", state->index + 1,
    config_enum_to_string(leds_interface_enum, options.interface),
    config_enum_to_string(leds_protocol_enum, options.protocol),
    options.count
  );

  LOG_INFO("leds%d: limit total=%u groups=%u / %u", state->index + 1,
    options.limit_total,
    options.limit_group, options.limit_groups
  );

  switch(options.interface) {
    case LEDS_INTERFACE_NONE:
      break;

  #if CONFIG_LEDS_SPI_ENABLED
    case LEDS_INTERFACE_SPI:
      if ((err = config_leds_spi(state, config, &options.spi))) {
        LOG_ERROR("leds%d: config_leds_spi", state->index + 1);
        return err;
      }

      if ((err = config_leds_gpio(state, config, LEDS_INTERFACE_SPI, &options.spi.gpio))) {
        LOG_ERROR("leds%d: config_leds_gpio", state->index + 1);
        return err;
      }

      break;
  #endif

  #if CONFIG_LEDS_UART_ENABLED
    case LEDS_INTERFACE_UART:
      if ((err = config_leds_uart(state, config, &options.uart))) {
        LOG_ERROR("leds%d: config_leds_uart", state->index + 1);
        return err;
      }

      if ((err = config_leds_gpio(state, config, LEDS_INTERFACE_UART, &options.uart.gpio))) {
        LOG_ERROR("leds%d: config_leds_gpio", state->index + 1);
        return err;
      }

      break;
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
  # if LEDS_I2S_INTERFACE_COUNT > 0
    case LEDS_INTERFACE_I2S0:
  # endif
  # if LEDS_I2S_INTERFACE_COUNT > 1
    case LEDS_INTERFACE_I2S1:
  # endif
      if ((err = config_leds_i2s(state, config, &options.i2s))) {
        LOG_ERROR("leds%d: config_leds_i2s", state->index + 1);
        return err;
      }

      if ((err = config_leds_gpio(state, config, options.interface, &options.i2s.gpio))) {
        LOG_ERROR("leds%d: config_leds_gpio", state->index + 1);
        return err;
      }

      break;
  #endif

    default:
      LOG_ERROR("leds%d: invalid interface=%d", state->index + 1, options.interface);
      return -1;
  }

  if ((err = leds_new(&state->leds, &options))) {
    LOG_ERROR("leds%d: leds_new", state->index + 1);
    return err;
  }

  return 0;
}
