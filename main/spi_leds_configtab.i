/* included via spi_leds_config.c */
const struct configtab SPI_LEDS_CONFIGTAB[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .value  = { .boolean = &SPI_LEDS_CONFIG.enabled },
  },
  { CONFIG_TYPE_ENUM, "protocol",
    .enum_values = spi_leds_protocol_enum,
    .value       = { .enum_value = &SPI_LEDS_CONFIG.protocol },
  },
  { CONFIG_TYPE_ENUM, "rate",
    .enum_values = spi_leds_rate_enum,
    .value       = { .enum_value = &SPI_LEDS_CONFIG.spi_clock },
  },
  { CONFIG_TYPE_UINT16, "count",
    .value  = { .uint16 = &SPI_LEDS_CONFIG.count },
  },

  { CONFIG_TYPE_ENUM, "gpio_mode",
    .enum_values = spi_leds_gpio_mode_enum,
    .value       = { .enum_value = &SPI_LEDS_CONFIG.gpio_mode },
  },
  { CONFIG_TYPE_UINT16, "gpio_pin",
    // TODO: max value
    .value  = { .uint16 = &SPI_LEDS_CONFIG.gpio_pin },
  },

  { CONFIG_TYPE_BOOL, "artnet_enabled",
    .value  = { .boolean = &SPI_LEDS_CONFIG.artnet_enabled },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe",
    .value  = { .uint16 = &SPI_LEDS_CONFIG.artnet_universe },
  },
  { CONFIG_TYPE_ENUM, "artnet_mode",
    .enum_values = spi_leds_artnet_mode_enum,
    .value       = { .enum_value = &SPI_LEDS_CONFIG.artnet_mode },
  },
  {}
};
