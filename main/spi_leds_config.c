#include "spi_leds.h"

struct spi_leds_config spi_leds_configs[SPI_LEDS_COUNT] = {
  [0] = {
    .protocol    = SPI_LEDS_PROTOCOL_APA102,
    .gpio_mode   = SPI_LEDS_GPIO_OFF,
    .artnet_mode = SPI_LEDS_BGR,
  },
  [1] = {
    .protocol    = SPI_LEDS_PROTOCOL_APA102,
    .gpio_mode   = SPI_LEDS_GPIO_OFF,
    .artnet_mode = SPI_LEDS_BGR,
  },
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

const struct config_enum spi_leds_gpio_mode_enum[] = {
  { "OFF",  SPI_LEDS_GPIO_OFF         },
  { "HIGH", SPI_GPIO_CS_ACTIVE_HIGH   },
  { "LOW",  SPI_GPIO_CS_ACTIVE_LOW    },
  {}
};

const struct config_enum spi_leds_artnet_mode_enum[] = {
  { "RGB", SPI_LEDS_RGB  },
  { "BGR", SPI_LEDS_BGR  },
  { "GRB", SPI_LEDS_GRB  },
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