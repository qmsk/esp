#include "leds.h"
#include "leds_config.h"
#include "leds_state.h"

#include <artnet.h>
#include <leds.h>

#include <logging.h>

#include <sdkconfig.h>

// approximation of how many LEDs will fit per ART-NET Universe
#define LEDS_ARTNET_UNIVERSE_LEDS (ARTNET_DMX_SIZE / 3)

struct leds_config leds_configs[LEDS_COUNT] = {};

const struct config_enum leds_interface_enum[] = {
  { "DEFAULT",  LEDS_INTERFACE_NONE   },
#if CONFIG_LEDS_SPI_ENABLED
  { "SPI",      LEDS_INTERFACE_SPI    },
#endif
#if CONFIG_LEDS_UART_ENABLED
  { "UART",     LEDS_INTERFACE_UART   },
#endif
#if CONFIG_LEDS_I2S_ENABLED
  { "I2S",      LEDS_INTERFACE_I2S    },
#endif
  {}
};

const struct config_enum leds_protocol_enum[] = {
  { "NONE",         0                         },
  { "APA102",       LEDS_PROTOCOL_APA102      },
  { "P9813",        LEDS_PROTOCOL_P9813       },
  { "WS2812B",      LEDS_PROTOCOL_WS2812B     },
  { "SK6812_GRBW",  LEDS_PROTOCOL_SK6812_GRBW },
  { "WS2811",       LEDS_PROTOCOL_WS2811      },
  {}
};

#if CONFIG_LEDS_UART_ENABLED
const struct config_enum leds_uart_port_enum[] = {
  { "",              -1       },
# if defined(UART_0) && CONFIG_ESP_CONSOLE_UART_NUM != 0
  { "UART0",         UART_0   },
# endif
# if defined(UART_1) && CONFIG_ESP_CONSOLE_UART_NUM != 1
  { "UART1",         UART_1   },
# endif
# if defined(UART_2) && CONFIG_ESP_CONSOLE_UART_NUM != 2
  { "UART2",         UART_2   },
# endif
  {},
};
#endif

const struct config_enum leds_spi_clock_enum[] = {
#if CONFIG_LEDS_SPI_ENABLED
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
#endif
  {}
};

const struct config_enum leds_gpio_mode_enum[] = {
  { "OFF",  -1                        },
#if CONFIG_LEDS_GPIO_ENABLED
  { "HIGH", GPIO_OUT_HIGH             },
  { "LOW",  GPIO_OUT_LOW              },
#endif
  {}
};

const struct config_enum leds_format_enum[] = {
  { "RGB",  LEDS_FORMAT_RGB  },
  { "BGR",  LEDS_FORMAT_BGR  },
  { "GRB",  LEDS_FORMAT_GRB  },
  { "RGBA", LEDS_FORMAT_RGBA },
  { "RGBW", LEDS_FORMAT_RGBW },
  {}
};

const struct config_enum leds_test_mode_enum[] = {
  { "BLACK",          TEST_MODE_BLACK         },
  { "CHASE",          TEST_MODE_CHASE         },
  { "BLACK_RED",      TEST_MODE_BLACK_RED     },
  { "RED_YELLOW",     TEST_MODE_RED_YELLOW    },
  { "YELLOW_GREEN",   TEST_MODE_YELLOW_GREEN  },
  { "GREEN_CYAN",     TEST_MODE_GREEN_CYAN    },
  { "CYAN_BLUE",      TEST_MODE_CYAN_BLUE     },
  { "BLUE_MAGENTA",   TEST_MODE_BLUE_MAGENTA  },
  { "MAGENTA_RED",    TEST_MODE_MAGENTA_RED   },
  { "RED_BLACK",      TEST_MODE_RED_BLACK     },
  { "END",            TEST_MODE_END           },
  {}
};

const struct config_enum leds_color_parameter_enum[] = {
  { "NONE",     LEDS_COLOR_NONE     },
  { "DIMMER",   LEDS_COLOR_DIMMER   },
  { "WHITE",    LEDS_COLOR_WHITE    },
  {}
};

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
      .interface  = config->interface,
      .protocol   = config->protocol,
      .count      = config->count,

  };
  int err;

  if (!config->interface) {
    options.interface = leds_interface_for_protocol(options.protocol);
  }

  LOG_INFO("leds%d: interface=%s protocol=%s count=%u", state->index,
    config_enum_to_string(leds_interface_enum, options.interface),
    config_enum_to_string(leds_protocol_enum, options.protocol),
    options.count
  );

  switch(options.interface) {
    case LEDS_INTERFACE_NONE:
      break;

  #if CONFIG_LEDS_UART_ENABLED
    case LEDS_INTERFACE_UART:
      options.uart = leds_uart[config->uart_port];
      // TODO: uart_pin_mutex?

      LOG_INFO("leds%d: uart port=%d -> uart=%p", state->index, config->uart_port, options.uart);
      break;
  #endif
  }

  if ((err = leds_new(&state->leds, &options))) {
    LOG_ERROR("leds%d: leds_new", state->index + 1);
    return err;
  }

  return 0;
}
