#ifndef __SPI_MASTER_H__
#define __SPI_MASTER_H__

#include <stddef.h>

enum spi_mode {
  SPI_MODE_CPOL_LOW    = 0x00,
  SPI_MODE_CPOL_HIGH   = 0x01,
  SPI_MODE_CPHA_LOW    = 0x00,
  SPI_MODE_CPHA_HIGH   = 0x02,

  SPI_MODE_MSB_FIRST   = 0x00,
  SPI_MODE_LSB_FIRST   = 0x04,
  SPI_MODE_LE_ORDER    = 0x00,
  SPI_MODE_BE_ORDER    = 0x08,

  SPI_MODE_0           = SPI_MODE_CPOL_LOW | SPI_MODE_CPHA_LOW,
  SPI_MODE_1           = SPI_MODE_CPOL_LOW | SPI_MODE_CPHA_HIGH,
  SPI_MODE_2           = SPI_MODE_CPOL_HIGH | SPI_MODE_CPHA_HIGH,
  SPI_MODE_3           = SPI_MODE_CPOL_HIGH | SPI_MODE_CPHA_LOW,
};

enum spi_clock {
  SPI_CLOCK_80MHZ       = 1,
  SPI_CLOCK_40MHZ       = 2,
  SPI_CLOCK_20MHZ       = 4,
  SPI_CLOCK_10MHZ       = 8,
  SPI_CLOCK_8MHZ        = 10,
  SPI_CLOCK_4MHZ        = 20,
  SPI_CLOCK_2MHZ        = 40,
  SPI_CLOCK_1MHZ        = 80,
};

enum spi_pins {
  SPI_PINS_CLK     = 0x01,
  SPI_PINS_MOSI    = 0x02,
  SPI_PINS_MISO    = 0x04,
  SPI_PINS_CS      = 0x08,

  SPI_PINS        = SPI_PINS_CLK | SPI_PINS_MOSI | SPI_PINS_MISO | SPI_PINS_CS,
};

struct spi_options {
  enum spi_mode mode;
  enum spi_clock clock;

  /* Should be set to SPI_PINS by default, to enable all pins */
  enum spi_pins pins;
};

struct spi_master;

/*
 * Setup SPI master mode.
 */
int spi_master_new(struct spi_master **spi_masterp, const struct spi_options options);

/*
 * Send up to 64 bytes of data out via SPI MOSI.
 *
 * Does not wait for transfer to complete when returning. When called, waits for any in-progress transfer to complete.
 *
 * The data MUST be uint32_t (4-byte) aligned.
 *
 * Returns bytes written, or <0 on error.
 */
int spi_master_write(struct spi_master *spi_master, void *data, size_t len);

#endif
