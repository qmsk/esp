const struct configtab DMX_OUTPUT_CONFIGTAB[] = {
 { CONFIG_TYPE_BOOL, "enabled",
   .bool_type = { .value = &DMX_OUTPUT_CONFIG.enabled },
 },
 /*
  * This should be a GPIO pin that's active low on boot, and used to drive the RS-485 transceiver's driver/output enable pin.
  * This allows supressing the UART1 bootloader debug output.
  */
 { CONFIG_TYPE_UINT16, "gpio_pin",
   .description = (
     "GPIO pin will be taken high to enable output once the UART1 TX output is safe."
   ),
   .uint16_type = { .value = &DMX_OUTPUT_CONFIG.gpio_pin, .max = GPIO_OUT_PIN_MAX },
 },
 { CONFIG_TYPE_ENUM, "gpio_mode",
   .description = "Multiplex between multiple active-high/low GPIO-controlled outputs",
   .enum_type = { .value = &DMX_OUTPUT_CONFIG.gpio_mode, .values = dmx_gpio_mode_enum },
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
