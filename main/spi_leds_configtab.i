/* included via spi_leds_config.c */
const struct configtab SPI_LEDS_CONFIGTAB[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &SPI_LEDS_CONFIG.enabled },
  },
  { CONFIG_TYPE_ENUM, "protocol",
    .enum_type = { .value = &SPI_LEDS_CONFIG.protocol, .values = spi_leds_protocol_enum },
  },
  { CONFIG_TYPE_ENUM, "rate",
    .description = "Longer cable runs can be noisier, and may need a slower rate to work reliably.",
    .enum_type = { .value = &SPI_LEDS_CONFIG.spi_clock, .values = spi_leds_rate_enum },
  },
  { CONFIG_TYPE_UINT16, "delay",
    .description = "Delay data signal transitions by system clock cycles to offset clock/data transitions and avoid coupling glitches.",
    .uint16_type = { .value = &SPI_LEDS_CONFIG.delay, .max = SPI_MODE_MOSI_DELAY_MAX },
  },
  { CONFIG_TYPE_UINT16, "count",
    .uint16_type = { .value = &SPI_LEDS_CONFIG.count },
  },

  { CONFIG_TYPE_ENUM, "gpio_mode",
    .description = "Multiplex between multiple active-high/low GPIO-controlled outputs",
    .enum_type = { .value = &SPI_LEDS_CONFIG.gpio_mode, .values = spi_leds_gpio_mode_enum },
  },
  { CONFIG_TYPE_UINT16, "gpio_pin",
    .description = "GPIO pin to activate when transmitting on this output",
    .uint16_type = { .value = &SPI_LEDS_CONFIG.gpio_pin, .max = GPIO_OUT_PIN_MAX },
  },

  { CONFIG_TYPE_BOOL, "artnet_enabled",
    .bool_type = { .value = &SPI_LEDS_CONFIG.artnet_enabled },
  },
  { CONFIG_TYPE_ENUM, "artnet_mode",
    .description = "Art-Net DMX channel mode",
    .enum_type = { .value = &SPI_LEDS_CONFIG.artnet_mode, .values = spi_leds_artnet_mode_enum },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe_start",
    .alias       = "artnet_universe",
    .description = "Output from artnet universe (0-15) within [artnet] net/subnet.",
    .uint16_type = { .value = &SPI_LEDS_CONFIG.artnet_universe_start, .max = ARTNET_UNIVERSE_MAX },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe_count",
    .description = "Output from multiple artnet universes, starting at artnet_universe_start.",
    .uint16_type = { .value = &SPI_LEDS_CONFIG.artnet_universe_count, .max = ARTNET_UNIVERSE_MAX },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe_step",
    .description = "Output from multiple non-consecutive artnet universes, at artnet_universe_start + artnet_universe_step",
    .uint16_type = { .value = &SPI_LEDS_CONFIG.artnet_universe_step, .max = 4 },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe_leds",
    .description = "Limit number of LEDs per artnet universe.",
    .uint16_type = { .value = &SPI_LEDS_CONFIG.artnet_universe_leds, .max = SPI_LEDS_ARTNET_UNIVERSE_LEDS },
  },
  { CONFIG_TYPE_UINT16, "artnet_dmx_addr",
    .description = "Start at DMX address within artnet universe. The first channel is adress 1",
    .uint16_type = { .value = &SPI_LEDS_CONFIG.artnet_dmx_addr, .max = 512 },
  },

  { CONFIG_TYPE_BOOL, "test_enabled",
    .description = "Output test patterns at boot",
    .bool_type = { .value = &SPI_LEDS_CONFIG.test_enabled },
  },

  {}
};
