#ifndef __SPI_MASTER_H__
#define __SPI_MASTER_H__

#include <stddef.h>

// not configurable, this is the only supported SPI host
#define SPI1_HOST 1

enum spi_mode {
  SPI_MODE_CPOL_LOW     = 0x00,
  SPI_MODE_CPOL_HIGH    = 0x01,
  SPI_MODE_CPHA_LOW     = 0x00,
  SPI_MODE_CPHA_HIGH    = 0x02,

  SPI_MODE_MSB_FIRST    = 0x00,
  SPI_MODE_LSB_FIRST    = 0x04,
  SPI_MODE_LE_ORDER     = 0x00,
  SPI_MODE_BE_ORDER     = 0x08,

  /* Delay MOSI by 0-7 system clock cycles */
  SPI_MODE_MOSI_DELAY_1     = 0x0100,
  SPI_MODE_MOSI_DELAY_2     = 0x0200,
  SPI_MODE_MOSI_DELAY_4     = 0x0400,
  SPI_MODE_MOSI_DELAY_MASK  = 0x0700,
  SPI_MODE_MOSI_DELAY_SHIFT = 8,
  SPI_MODE_MOSI_DELAY_MAX   = 7,

  /* Delay MISO by 0-7 system clock cycles */
  SPI_MODE_MISO_DELAY_1     = 0x1000,
  SPI_MODE_MISO_DELAY_2     = 0x2000,
  SPI_MODE_MISO_DELAY_4     = 0x4000,
  SPI_MODE_MISO_DELAY_MASK  = 0x7000,
  SPI_MODE_MISO_DELAY_SHIFT = 12,
  SPI_MODE_MISO_DELAY_MAX   = 7,

  SPI_MODE_FLAGS       = 0x0ffff,
  SPI_MODE_SET         = 0x10000,

  SPI_MODE_0           = SPI_MODE_CPOL_LOW | SPI_MODE_CPHA_LOW,
  SPI_MODE_1           = SPI_MODE_CPOL_LOW | SPI_MODE_CPHA_HIGH,
  SPI_MODE_2           = SPI_MODE_CPOL_HIGH | SPI_MODE_CPHA_LOW,
  SPI_MODE_3           = SPI_MODE_CPOL_HIGH | SPI_MODE_CPHA_HIGH,
};

enum spi_clock {
  SPI_CLOCK_80MHZ       = 1,
  SPI_CLOCK_40MHZ       = 2,
  SPI_CLOCK_20MHZ       = 4,
  SPI_CLOCK_10MHZ       = 8,
  SPI_CLOCK_8MHZ        = 10,
  SPI_CLOCK_5MHZ        = 16,
  SPI_CLOCK_4MHZ        = 20,
  SPI_CLOCK_2MHZ        = 40,
  SPI_CLOCK_1MHZ        = 80,
  SPI_CLOCK_500KHZ      = 160,
  SPI_CLOCK_200KHZ      = 400,
  SPI_CLOCK_100KHZ      = 800,
  SPI_CLOCK_50KHZ       = 1600,
  SPI_CLOCK_20KHZ       = 4000,
  SPI_CLOCK_10KHZ       = 8000,
  SPI_CLOCK_1KHZ        = 80000,
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

struct spi_write_options {
  /* ignored if 0, use SPI_MODE_SET to override SPI_MODE_0 */
  enum spi_mode mode;

  /* ignored if 0 */
  enum spi_clock clock;
};

struct spi_master;

/*
 * Setup SPI master mode.
 */
int spi_master_new(struct spi_master **spi_masterp, const struct spi_options options);

/*
 * Lock spi master, wait done, set mode
 *
 * The spi master is reconfigured to use given options, if given and different from previous configuration.
 */
int spi_master_open(struct spi_master *spi_master, struct spi_write_options options);

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

/*
 * wait write done
 */
int spi_master_flush(struct spi_master *spi_master);

/*
 * release
 *
 * does not wait for TX done.
 */
int spi_master_close(struct spi_master *spi_master);


#endif
