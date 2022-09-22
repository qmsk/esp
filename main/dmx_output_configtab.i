const struct configtab DMX_OUTPUT_CONFIGTAB[] = {
 { CONFIG_TYPE_BOOL, "enabled",
   .bool_type = { .value = &DMX_OUTPUT_CONFIG.enabled },
 },
 /*
  * This should be a GPIO pin that's active low on boot, and used to drive the RS-485 transceiver's driver/output enable pin.
  * This allows supressing the UART1 bootloader debug output.
  */
 { CONFIG_TYPE_ENUM, "gpio_type",
   .description = "Use built-in (HOST) GPIO pins, or external I2C GPIO expander via i2c-gpio config",
   .enum_type = { .value = &DMX_OUTPUT_CONFIG.gpio_type, .values = gpio_type_enum, .default_value = GPIO_TYPE_HOST },
 },
 { CONFIG_TYPE_ENUM, "gpio_mode",
   .description = "Multiplex between multiple active-high/low GPIO-controlled outputs",
   .enum_type = { .value = &DMX_OUTPUT_CONFIG.gpio_mode, .values = dmx_gpio_mode_enum, .default_value = DMX_GPIO_MODE_DISABLED },
 },
{ CONFIG_TYPE_UINT16, "gpio_pin",
  .description = (
    "GPIO pin will be taken high to enable output once the UART1 TX output is safe."
  ),
  .count = &DMX_OUTPUT_CONFIG.gpio_count, .size = DMX_GPIO_COUNT,
  .uint16_type = { .value = DMX_OUTPUT_CONFIG.gpio_pins, .max = GPIO_PIN_MAX },
},

 { CONFIG_TYPE_BOOL, "artnet_enabled",
   .bool_type = { .value = &DMX_OUTPUT_CONFIG.artnet_enabled },
 },
 { CONFIG_TYPE_UINT16, "artnet_universe",
   .description = "Output from universe (0-15) within [artnet] net/subnet.",
   .uint16_type = { .value = &DMX_OUTPUT_CONFIG.artnet_universe, .max = ARTNET_UNIVERSE_MAX },
 },
 {}
};
