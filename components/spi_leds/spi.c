#include "spi_leds.h"

#include <logging.h>

int spi_leds_tx_spi(const struct spi_leds_options *options, enum spi_mode spi_mode, void *buf, size_t size)
{
  struct spi_write_options spi_write_options = {
    .mode   = options->spi_mode_bits | spi_mode | SPI_MODE_SET,
    .clock  = options->spi_clock,
  };
  uint8_t *ptr = buf;
  unsigned len = size;
  int ret, err;

  if ((err = spi_master_open(options->spi_master, spi_write_options))) {
    LOG_ERROR("spi_master_open");
    return err;
  }

  if (options->gpio_out) {
    gpio_out_set(options->gpio_out, options->gpio_out_pins);
  }

  while (len) {
    if ((ret = spi_master_write(options->spi_master, ptr, len)) < 0) {
      LOG_ERROR("spi_master_write");
      goto error;
    }

    LOG_DEBUG("spi_leds=%p write %p @ %u -> %d", spi_leds, ptr, len, ret);

    ptr += ret;
    len -= ret;
  }

  // wait for write TX to complete before clearing gpio to release bus
  if ((ret = spi_master_flush(options->spi_master)) < 0) {
    LOG_ERROR("spi_master_flush");
    goto error;
  }

error:
  if (options->gpio_out) {
    gpio_out_clear(options->gpio_out);
  }

  if ((err = spi_master_close(options->spi_master))) {
    LOG_ERROR("spi_master_close");
    return err;
  }

  return ret;
}
