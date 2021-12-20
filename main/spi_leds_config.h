#pragma once

#include <config.h>

#define SPI_LEDS_COUNT 4

extern const struct configtab spi_leds_configtab0[];
extern const struct configtab spi_leds_configtab1[];
extern const struct configtab spi_leds_configtab2[];
extern const struct configtab spi_leds_configtab3[];

extern const struct config_enum spi_leds_interface_enum[];
extern const struct config_enum spi_leds_protocol_enum[];
extern const struct config_enum spi_leds_rate_enum[];
extern const struct config_enum spi_leds_gpio_mode_enum[];
extern const struct config_enum spi_leds_artnet_mode_enum[];
