/* included via leds_config.c */
const struct configtab LEDS_CONFIGTAB[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &LEDS_CONFIG.enabled },
  },
  { CONFIG_TYPE_ENUM, "interface",
    .enum_type = { .value = &LEDS_CONFIG.interface, .values = leds_interface_enum },
  },
  { CONFIG_TYPE_ENUM, "protocol",
    .enum_type = { .value = &LEDS_CONFIG.protocol, .values = leds_protocol_enum },
  },
  { CONFIG_TYPE_UINT16, "count",
    .description = "Number of LED pixels",
    .uint16_type = { .value = &LEDS_CONFIG.count, .max = LEDS_COUNT_MAX },
  },
  { CONFIG_TYPE_UINT16, "limit_total",
    .alias = "limit",
    .description = (
      "Apply total power limit for the maximum number of LED pixels at full brightness across all LEDs."
      "Default 0 -> no total limit"
    ),
    .uint16_type = { .value = &LEDS_CONFIG.limit_total, .max = LEDS_COUNT_MAX },
  },
  { CONFIG_TYPE_UINT16, "limit_group",
    .description = (
      "Apply per-group power limit for the maximum number of LED pixels at full brightness within each group of consecutive LEDs."
      "Default 0 -> no per-group limit"
    ),
    .uint16_type = { .value = &LEDS_CONFIG.limit_group, .max = LEDS_COUNT_MAX },
  },
  { CONFIG_TYPE_UINT16, "limit_groups",
    .description = (
      "Number of groups to split consecutive LEDs across for per-group power limiting."
      "Default 0 -> no groups"
    ),
    .uint16_type = { .value = &LEDS_CONFIG.limit_groups, .max = LEDS_LIMIT_GROUPS_MAX },
  },

#if CONFIG_LEDS_SPI_ENABLED
  { CONFIG_TYPE_ENUM, "spi_clock",
    .alias = "spi_rate",
    .description = "Longer cable runs can be noisier, and may need a slower rate to work reliably.",
    .enum_type = { .value = &LEDS_CONFIG.spi_clock, .values = leds_spi_clock_enum, .default_value = SPI_CLOCK_DEFAULT },
  },
# if CONFIG_IDF_TARGET_ESP8266
  { CONFIG_TYPE_UINT16, "spi_delay",
    .alias = "delay",
    .description = "Delay data signal transitions by system clock cycles to offset clock/data transitions and avoid coupling glitches.",
    .uint16_type = { .value = &LEDS_CONFIG.spi_delay, .max = SPI_MODE_MOSI_DELAY_MAX },
  },
# else
  { CONFIG_TYPE_ENUM, "spi_cs_mode",
    .description = "Output SPI Chip Select when transmitting.",
    .enum_type = { .value = &LEDS_CONFIG.spi_cs_mode, .values = leds_spi_cs_mode_enum, .default_value = LEDS_SPI_CS_MODE_DISABLED },
  },
  { CONFIG_TYPE_UINT16, "spi_cs_io",
    .description = "Output SPI Chip Select to IO pin when transmitting.",
    .uint16_type = { .value = &LEDS_CONFIG.spi_cs_io, .max = (SOC_GPIO_PIN_COUNT - 1) },
  },
# endif
#endif

#if CONFIG_LEDS_I2S_ENABLED
  { CONFIG_TYPE_ENUM, "i2s_clock",
    .alias = "i2s_clock",
    .description = "Output I2S bit rate. Only used for protocols with a separate clock/data.",
    .enum_type = { .value = &LEDS_CONFIG.i2s_clock, .values = leds_i2s_clock_enum, .default_value = I2S_CLOCK_DEFAULT },
  },
  { CONFIG_TYPE_UINT16, "i2s_data_repeat",
    .description = (
      "Split LEDs across parallel I2S data signals to different GPIOs.\n"
      "\t0 (default, compat) -> automatically determine based on number of configured data pins.\n"
      "\t1 -> serial mode with a single data signal, optionally multiple copies of the same leds on each gpio pin.\n"
      "\tN -> parallel mode with multiple data signals, separate leds on each gpio pin.\n"
    ),
    .uint16_type = { .value = &LEDS_CONFIG.i2s_data_repeat, .max = LEDS_I2S_REPEAT_MAX },
  },
# if LEDS_I2S_PARALLEL_ENABLED
  { CONFIG_TYPE_UINT16, "i2s_data_width",
    .description = (
      "Split LEDs across parallel I2S data signals to different GPIOs.\n"
      "\t0 (default, compat) -> automatically determine based on number of configured data pins.\n"
      "\t1 -> serial mode with a single data signal, optionally multiple copies of the same leds on each gpio pin.\n"
      "\tN -> parallel mode with multiple data signals, separate leds on each gpio pin.\n"
    ),
    .uint16_type = { .value = &LEDS_CONFIG.i2s_data_width, .max = LEDS_I2S_PARALLEL_MAX },
  },
# endif
# if LEDS_I2S_GPIO_PINS_ENABLED
  { CONFIG_TYPE_UINT16, "i2s_clock_pin",
    .description = "Output I2S bit clock to GPIO pin. Only used for protocols with a separate clock/data.",
    .count  = &LEDS_CONFIG.i2s_clock_pin_count, .size = LEDS_I2S_GPIO_PINS_SIZE,
    .uint16_type = { .value = LEDS_CONFIG.i2s_clock_pins, .max = (GPIO_NUM_MAX - 1) },
  },
  { CONFIG_TYPE_UINT16, "i2s_data_pin",
    .alias = "i2s_gpio_pin",
    .description = (
      "Output serial/parallel I2S data to GPIO pin. 0 -> skip this pin.\n"
      "\tIf there are more gpio pins than i2s_data_width, the remaining gpio pins loop over to output copies of the first N signals.\n"
    ),
    .count  = &LEDS_CONFIG.i2s_data_pin_count, .size = LEDS_I2S_GPIO_PINS_SIZE,
    .uint16_type = { .value = LEDS_CONFIG.i2s_data_pins, .max = (GPIO_NUM_MAX - 1) },
  },
  { CONFIG_TYPE_UINT16, "i2s_data_inv_pin",
    .description = (
      "Output inverted copy of I2S data to GPIO pin. 0 -> skip this pin.\n"
      "\tIf there are more gpio pins than i2s_data_width, the remaining gpio pins loop over to output copies of the first N signals.\n"
    ),
    .count  = &LEDS_CONFIG.i2s_data_inv_pin_count, .size = LEDS_I2S_GPIO_PINS_SIZE,
    .uint16_type = { .value = LEDS_CONFIG.i2s_data_inv_pins, .max = (GPIO_NUM_MAX - 1) },
  },
# endif
#endif

#if CONFIG_LEDS_GPIO_ENABLED
  { CONFIG_TYPE_ENUM, "gpio_type",
    .description = "Use built-in (HOST) GPIO pins, or external I2C GPIO expander via i2c-gpio config",
    .enum_type = { .value = &LEDS_CONFIG.gpio_type, .values = gpio_type_enum, .default_value = GPIO_TYPE_HOST },
  },
  { CONFIG_TYPE_ENUM, "gpio_mode",
    .description = (
      "Control multiple active-high/low GPIO-controlled outputs based on LEDS state."
      " SETUP -> active once interface is first setup, remains active while idle."
      " ACTIVE -> active when interface is setup and ready to transmit, inactive once idle."
    ),
    .enum_type = { .value = &LEDS_CONFIG.gpio_mode, .values = leds_gpio_mode_enum, .default_value = LEDS_CONFIG_GPIO_MODE_DISABLED },
  },
  { CONFIG_TYPE_UINT16, "gpio_pin",
    .description = "GPIO pin to activate when output is setup/active",
    .count = &LEDS_CONFIG.gpio_count, .size = LEDS_GPIO_SIZE,
    .uint16_type = { .value = LEDS_CONFIG.gpio_pin, .max = GPIO_PIN_MAX },
  },
#endif

  { CONFIG_TYPE_UINT16, "update_timeout",
    .description = "Update LED outputs after given milliseconds without any updates. Default 0 -> hold output idle.",
    .uint16_type = { .value = &LEDS_CONFIG.update_timeout },
  },

  { CONFIG_TYPE_BOOL, "test_enabled",
    .description = "Enable test pattern output, active when TEST button pressed",
    .bool_type = { .value = &LEDS_CONFIG.test_enabled, .default_value = true },
  },
  { CONFIG_TYPE_ENUM, "test_mode",
    .description = "Output test pattern at boot",
    .enum_type = { .value = &LEDS_CONFIG.test_mode, .values = leds_test_mode_enum, .default_value = TEST_MODE_NONE },
  },
  { CONFIG_TYPE_BOOL, "test_auto",
    .description = "Cycle test patterns at boot",
    .bool_type = { .value = &LEDS_CONFIG.test_auto, .default_value = false },
  },

  { CONFIG_TYPE_BOOL, "artnet_enabled",
    .bool_type = { .value = &LEDS_CONFIG.artnet_enabled },
  },
  { CONFIG_TYPE_UINT16, "artnet_net",
    .description = "Art-Net Net address: 0-127",
    .uint16_type = { .value = &LEDS_CONFIG.artnet_net, .max = ARTNET_NET_MAX },
  },
  { CONFIG_TYPE_UINT16, "artnet_subnet",
    .description = "Art-Net Sub-Net address: 0-16",
    .uint16_type = { .value = &LEDS_CONFIG.artnet_subnet, .max = ARTNET_SUBNET_MAX },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe_start",
    .alias       = "artnet_universe",
    .description = "Output from Art-Net DMX universe (0-15) within [artnet] net/subnet.",
    .uint16_type = { .value = &LEDS_CONFIG.artnet_universe_start, .max = ARTNET_UNIVERSE_MAX },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe_count",
    .description = "Output from multiple Art-Net DMX universes. Default 0 -> automatic, enough to fit all LEDs",
    .uint16_type = { .value = &LEDS_CONFIG.artnet_universe_count, .max = ARTNET_OUTPUTS_MAX },
    .validate_func = validate_artnet_universe_count,
    .ctx = &LEDS_CONFIG,
  },
  { CONFIG_TYPE_UINT16, "artnet_universe_step",
    .description = "Output from non-consecutive Art-Net DMX universes, at artnet_universe_start + i*artnet_universe_step. Default 1, special-case 0 -> output multiple copies of the same universe",
    .uint16_type = { .value = &LEDS_CONFIG.artnet_universe_step, .default_value = 1, .max = 4 },
  },
  { CONFIG_TYPE_UINT16, "artnet_dmx_addr",
    .description = "Start at DMX address within Art-Net DMX universe. Default 0 -> first channel, the first channel is at adress 1",
    .uint16_type = { .value = &LEDS_CONFIG.artnet_dmx_addr, .max = ARTNET_DMX_SIZE },
  },
  { CONFIG_TYPE_UINT16, "artnet_dmx_leds",
    .alias       = "artnet_universe_leds",
    .description = "Limit number of LEDs per Art-Net DMX universe. Default 0 -> as many LEDs as will fit with the given format (170 or 128).",
    .uint16_type = { .value = &LEDS_CONFIG.artnet_dmx_leds, .max = LEDS_ARTNET_UNIVERSE_LEDS },
    .validate_func = validate_artnet_dmx_leds,
    .ctx = &LEDS_CONFIG,
  },
  { CONFIG_TYPE_UINT16, "artnet_dmx_timeout",
    .description = "Clear LED outputs after given milliseconds without any DMX updates. Default 0 -> hold output.",
    .uint16_type = { .value = &LEDS_CONFIG.artnet_dmx_timeout },
  },
  { CONFIG_TYPE_UINT16, "artnet_sync_timeout",
    .description = "Sync LED outputs after given milliseconds from first updated universe, or once all universes updated. Default 0 -> sync on each updated universe.",
    .uint16_type = { .value = &LEDS_CONFIG.artnet_sync_timeout },
  },
  { CONFIG_TYPE_ENUM, "artnet_leds_format",
    .alias       = "artnet_mode",
    .description = "LED color format for Art-Net DMX channels",
    .enum_type = { .value = &LEDS_CONFIG.artnet_leds_format, .values = leds_format_enum },
  },
  { CONFIG_TYPE_UINT16, "artnet_leds_segment",
    .description = "Control multiple consecutive LEDs per Art-Net DMX channel. Default 0 -> set individual LEDs",
    .uint16_type = { .value = &LEDS_CONFIG.artnet_leds_segment },
  },
  { CONFIG_TYPE_UINT16, "artnet_leds_group",
    .description = "Group multiple consecutive LEDs into the same Art-NET DMX universe, optionally with common format parameters. Default 0 -> individual LEDs",
    .uint16_type = { .value = &LEDS_CONFIG.artnet_leds_group },
    .validate_func = validate_artnet_leds_group,
    .ctx = &LEDS_CONFIG,
  },

  { CONFIG_TYPE_BOOL, "sequence_enabled",
    .description = "Output LED sequence frames, requires leds-sequence config",
    .bool_type = { .value = &LEDS_CONFIG.sequence_enabled },
  },
  { CONFIG_TYPE_ENUM, "sequence_format",
    .description = "LED color format for sequence channels",
    .enum_type = { .value = &LEDS_CONFIG.sequence_format, .values = leds_format_enum },
  },
  { CONFIG_TYPE_UINT16, "sequence_channel_start",
    .description = "Output LED data starting from specific channel. Default 0 -> all channels",
    .uint16_type = { .value = &LEDS_CONFIG.sequence_channel_start },
  },
  { CONFIG_TYPE_UINT16, "sequence_channel_count",
    .description = "Output LED data from limited number of channels. Default 0 -> all channels",
    .uint16_type = { .value = &LEDS_CONFIG.sequence_channel_count },
  },
  { CONFIG_TYPE_UINT16, "sequence_leds_count",
    .description = "Limit number of LEDs to control. Default 0 -> all LEDs",
    .uint16_type = { .value = &LEDS_CONFIG.sequence_leds_count },
  },
  { CONFIG_TYPE_UINT16, "sequence_leds_offset",
    .description = "Set first LED to control. Default 0 -> all LEDs",
    .uint16_type = { .value = &LEDS_CONFIG.sequence_leds_offset },
  },
  { CONFIG_TYPE_UINT16, "sequence_leds_segment",
    .description = "Control multiple consecutive LEDs per sequence channel. Default 0 -> control individual LEDs",
    .uint16_type = { .value = &LEDS_CONFIG.sequence_leds_segment },
  },

  {}
};
