#pragma once

#include <leds.h>

#if CONFIG_LEDS_SPI_ENABLED && CONFIG_IDF_TARGET_ESP8266
// using custom spi_master driver
# include <spi_master.h>

  struct leds_interface_spi {
    struct spi_master *spi_master;
    struct spi_write_options options;
  };


#elif CONFIG_LEDS_SPI_ENABLED
// using esp-idf spi_master driver
# include <driver/spi_master.h>

# define SPI_MODE_0 (0)
# define SPI_MODE_1 (1)
# define SPI_MODE_2 (2)
# define SPI_MODE_3 (3)

  struct leds_interface_spi {
    spi_device_handle_t device;
  };

#endif

int leds_interface_spi_init(struct leds_interface_spi *interface, const struct leds_options *options, void **bufp, size_t buf_size, int spi_mode);
int leds_interface_spi_tx(struct leds_interface_spi *interface, const struct leds_options *options, void *buf, size_t size);
