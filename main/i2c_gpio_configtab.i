const struct configtab I2C_GPIO_CONFIGTAB[] = {
  { CONFIG_TYPE_ENUM, "type",
    .description = "GPIO interface type.",
    .enum_type = { .value = &I2C_GPIO_CONFIG.type, .values = i2c_gpio_type_enum, .default_value = GPIO_TYPE_HOST },
  },
  { CONFIG_TYPE_UINT16, "addr",
    .description = "I2C GPIO expander address.",
    .uint16_type = { .value = &I2C_GPIO_CONFIG.addr, .max = GPIO_I2C_ADDR_MAX },
  },
  { CONFIG_TYPE_UINT16, "int_pin",
    .description = "I2C GPIO expander address.",
    .uint16_type = { .value = &I2C_GPIO_CONFIG.int_pin, .max = GPIO_I2C_ADDR_MAX },
  },
  {},
};
