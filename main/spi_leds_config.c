#include "spi_leds.h"

#include <artnet.h>

struct spi_leds_config spi_leds_configs[SPI_LEDS_COUNT] = {
  [0] = {
    .protocol    = SPI_LEDS_PROTOCOL_APA102,
    .spi_clock   = SPI_LEDS_CLOCK,
    .gpio_mode   = SPI_LEDS_GPIO_OFF,
    .artnet_mode = SPI_LEDS_FORMAT_BGR,
    .artnet_universe_count  = 1,
    .artnet_universe_step   = 1,
    .artnet_universe_leds   = SPI_LEDS_ARTNET_UNIVERSE_LEDS,
  },
  [1] = {
    .protocol    = SPI_LEDS_PROTOCOL_APA102,
    .spi_clock   = SPI_LEDS_CLOCK,
    .gpio_mode   = SPI_LEDS_GPIO_OFF,
    .artnet_mode = SPI_LEDS_FORMAT_BGR,
    .artnet_universe_count  = 1,
    .artnet_universe_step   = 1,
    .artnet_universe_leds   = SPI_LEDS_ARTNET_UNIVERSE_LEDS,
  },
  [2] = {
    .protocol    = SPI_LEDS_PROTOCOL_APA102,
    .spi_clock   = SPI_LEDS_CLOCK,
    .gpio_mode   = SPI_LEDS_GPIO_OFF,
    .artnet_mode = SPI_LEDS_FORMAT_BGR,
    .artnet_universe_count  = 1,
    .artnet_universe_step   = 1,
    .artnet_universe_leds   = SPI_LEDS_ARTNET_UNIVERSE_LEDS,
  },
  [3] = {
    .protocol    = SPI_LEDS_PROTOCOL_APA102,
    .spi_clock   = SPI_LEDS_CLOCK,
    .gpio_mode   = SPI_LEDS_GPIO_OFF,
    .artnet_mode = SPI_LEDS_FORMAT_BGR,
    .artnet_universe_count  = 1,
    .artnet_universe_step   = 1,
    .artnet_universe_leds   = SPI_LEDS_ARTNET_UNIVERSE_LEDS,
  },
};

const struct config_enum spi_leds_interface_enum[] = {
  { "DEFAULT",  0                         },
  { "SPI",      SPI_LEDS_INTERFACE_SPI    },
  { "UART",     SPI_LEDS_INTERFACE_UART   },
  {}
};

const struct config_enum spi_leds_protocol_enum[] = {
  { "APA102",       SPI_LEDS_PROTOCOL_APA102    },
  { "P9813",        SPI_LEDS_PROTOCOL_P9813     },
  { "WS2812B",      SPI_LEDS_PROTOCOL_WS2812B   },
  { "SK6812_GRBW",  SPI_LEDS_PROTOCOL_SK6812_GRBW   },
  { "WS2811",       SPI_LEDS_PROTOCOL_WS2811    },
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

const struct config_enum spi_leds_gpio_mode_enum[] = {
  { "OFF",  SPI_LEDS_GPIO_OFF         },
  { "HIGH", GPIO_OUT_HIGH             },
  { "LOW",  GPIO_OUT_LOW              },
  {}
};

const struct config_enum spi_leds_artnet_mode_enum[] = {
  { "RGB",  SPI_LEDS_FORMAT_RGB  },
  { "BGR",  SPI_LEDS_FORMAT_BGR  },
  { "GRB",  SPI_LEDS_FORMAT_GRB  },
  { "RGBA", SPI_LEDS_FORMAT_RGBA },
  { "RGBW", SPI_LEDS_FORMAT_RGBW },
  {}
};

#define SPI_LEDS_CONFIGTAB spi_leds_configtab0
#define SPI_LEDS_CONFIG spi_leds_configs[0]
#include "spi_leds_configtab.i"
#undef SPI_LEDS_CONFIGTAB
#undef SPI_LEDS_CONFIG

#define SPI_LEDS_CONFIGTAB spi_leds_configtab1
#define SPI_LEDS_CONFIG spi_leds_configs[1]
#include "spi_leds_configtab.i"
#undef SPI_LEDS_CONFIGTAB
#undef SPI_LEDS_CONFIG

#define SPI_LEDS_CONFIGTAB spi_leds_configtab2
#define SPI_LEDS_CONFIG spi_leds_configs[2]
#include "spi_leds_configtab.i"
#undef SPI_LEDS_CONFIGTAB
#undef SPI_LEDS_CONFIG

#define SPI_LEDS_CONFIGTAB spi_leds_configtab3
#define SPI_LEDS_CONFIG spi_leds_configs[3]
#include "spi_leds_configtab.i"
#undef SPI_LEDS_CONFIGTAB
#undef SPI_LEDS_CONFIG
