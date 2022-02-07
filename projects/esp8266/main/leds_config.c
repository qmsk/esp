#include "leds.h"

#include <artnet.h>

struct leds_config leds_configs[LEDS_COUNT] = {
  [0] = {
    .protocol    = LEDS_PROTOCOL_APA102,
    .spi_clock   = LEDS_SPI_CLOCK,
    .gpio_mode   = LEDS_GPIO_OFF,
    .artnet_mode = LEDS_FORMAT_BGR,
    .artnet_universe_count  = 1,
    .artnet_universe_step   = 1,
    .artnet_universe_leds   = LEDS_ARTNET_UNIVERSE_LEDS,
  },
  [1] = {
    .protocol    = LEDS_PROTOCOL_APA102,
    .spi_clock   = LEDS_SPI_CLOCK,
    .gpio_mode   = LEDS_GPIO_OFF,
    .artnet_mode = LEDS_FORMAT_BGR,
    .artnet_universe_count  = 1,
    .artnet_universe_step   = 1,
    .artnet_universe_leds   = LEDS_ARTNET_UNIVERSE_LEDS,
  },
  [2] = {
    .protocol    = LEDS_PROTOCOL_APA102,
    .spi_clock   = LEDS_SPI_CLOCK,
    .gpio_mode   = LEDS_GPIO_OFF,
    .artnet_mode = LEDS_FORMAT_BGR,
    .artnet_universe_count  = 1,
    .artnet_universe_step   = 1,
    .artnet_universe_leds   = LEDS_ARTNET_UNIVERSE_LEDS,
  },
  [3] = {
    .protocol    = LEDS_PROTOCOL_APA102,
    .spi_clock   = LEDS_SPI_CLOCK,
    .gpio_mode   = LEDS_GPIO_OFF,
    .artnet_mode = LEDS_FORMAT_BGR,
    .artnet_universe_count  = 1,
    .artnet_universe_step   = 1,
    .artnet_universe_leds   = LEDS_ARTNET_UNIVERSE_LEDS,
  },
};

const struct config_enum leds_interface_enum[] = {
  { "DEFAULT",  0                         },
  { "SPI",      LEDS_INTERFACE_SPI    },
  { "UART",     LEDS_INTERFACE_UART   },
  { "I2S",      LEDS_INTERFACE_I2S    },
  {}
};

const struct config_enum leds_protocol_enum[] = {
  { "APA102",       LEDS_PROTOCOL_APA102    },
  { "P9813",        LEDS_PROTOCOL_P9813     },
  { "WS2812B",      LEDS_PROTOCOL_WS2812B   },
  { "SK6812_GRBW",  LEDS_PROTOCOL_SK6812_GRBW   },
  { "WS2811",       LEDS_PROTOCOL_WS2811    },
  {}
};

const struct config_enum leds_rate_enum[] = {
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

const struct config_enum leds_gpio_mode_enum[] = {
  { "OFF",  LEDS_GPIO_OFF         },
  { "HIGH", GPIO_OUT_HIGH             },
  { "LOW",  GPIO_OUT_LOW              },
  {}
};

const struct config_enum leds_artnet_mode_enum[] = {
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
