#pragma once

enum spi_leds_artnet_mode {
  SPI_LEDS_RGB,
  SPI_LEDS_BGR,
  SPI_LEDS_GRB,
};

int init_spi_leds_artnet(uint16_t universe, enum spi_leds_artnet_mode mode);
