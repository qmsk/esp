/* included via spi_leds_config.c */
const struct configtab SPI_LEDS_CONFIGTAB[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &SPI_LEDS_CONFIG.enabled },
  },
  { CONFIG_TYPE_ENUM, "protocol",
    .enum_type = { .value = &SPI_LEDS_CONFIG.protocol, .values = spi_leds_protocol_enum },
  },
  { CONFIG_TYPE_ENUM, "rate",
    .enum_type = { .value = &SPI_LEDS_CONFIG.spi_clock, .values = spi_leds_rate_enum },
  },
  { CONFIG_TYPE_UINT16, "count",
    .uint16_type = { .value = &SPI_LEDS_CONFIG.count },
  },

  { CONFIG_TYPE_ENUM, "gpio_mode",
    .enum_type = { .value = &SPI_LEDS_CONFIG.gpio_mode, .values = spi_leds_gpio_mode_enum },
  },
  { CONFIG_TYPE_UINT16, "gpio_pin",
    // TODO: max value
    .uint16_type = { .value = &SPI_LEDS_CONFIG.gpio_pin },
  },

  { CONFIG_TYPE_BOOL, "artnet_enabled",
    .bool_type = { .value = &SPI_LEDS_CONFIG.artnet_enabled },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe",
    .uint16_type = { .value = &SPI_LEDS_CONFIG.artnet_universe },
  },
  { CONFIG_TYPE_ENUM, "artnet_mode",
    .enum_type = { .value = &SPI_LEDS_CONFIG.artnet_mode, .values = spi_leds_artnet_mode_enum },
  },
  {}
};
