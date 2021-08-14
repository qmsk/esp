#include <spi_master.h>
#include "spi_master.h"
#include "spi_dev.h"
#include <logging.h>

#define SPI_BYTES_MAX 64

static inline void spi_master_mosi(struct spi_master *spi_master, uint32_t *data, unsigned bits)
{
  LOG_DEBUG("spi_master=%p data=%p bits=%u", spi_master, data, bits);

  SPI_DEV.user1.usr_mosi_bitlen = bits - 1;

  for (unsigned i = 0; i < bits / 32 && i < 16; i++) {
    SPI_DEV.data_buf[i] = data[i];
  }
}

static inline void spi_master_start(struct spi_master *spi_master)
{
  LOG_DEBUG("spi_master=%p", spi_master);

  SPI_DEV.cmd.usr = 1;
}

static int spi_master_reconfigure(struct spi_master *spi_master, struct spi_write_options options)
{
  int err = 0;

  if (options.mode && (options.mode & SPI_MODE_FLAGS) != spi_master->mode) {
    LOG_DEBUG("set mode=%02x", options.mode);

    if ((err = spi_master_mode(spi_master, options.mode))) {
      LOG_ERROR("spi_master_mode");
      return err;
    }
  }

  if (options.clock && options.clock != spi_master->clock) {
    LOG_DEBUG("set clock=%d", options.clock);

    if ((err = spi_master_clock(spi_master, options.clock))) {
      LOG_ERROR("spi_master_clock");
      return err;
    }
  }

  return err;
}

int spi_master_write(struct spi_master *spi_master, void *data, size_t len, struct spi_write_options options)
{
  int ret;

  LOG_DEBUG("spi_master=%p data=%p len=%u", spi_master, data, len);

  if (len > SPI_BYTES_MAX) {
    len = SPI_BYTES_MAX;
  }

  if (!xSemaphoreTake(spi_master->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  if ((ret = spi_master_interrupt_wait_trans(spi_master, portMAX_DELAY))) {
    LOG_ERROR("spi_master_interrupt_wait_trans");
    goto error;
  }

  if ((ret = spi_master_reconfigure(spi_master, options))) {
    LOG_ERROR("spi_master_reconfigure");
    goto error;
  }

  // seutp CS outputs
  spi_master_gpio_set(spi_master, options.gpio);

  // usr command structure: mosi only
  SPI_DEV.user.usr_command = 0;
  SPI_DEV.user.usr_addr = 0;
  SPI_DEV.user.usr_dummy = 0;
  SPI_DEV.user.usr_mosi = 1;
  SPI_DEV.user.usr_miso = 0;

  // TODO: assumes uint32_t alignment, support non-aligned?
  spi_master_mosi(spi_master, data, len * 8);

  spi_master_start(spi_master);

  ret = len;

error:
  if (!xSemaphoreGive(spi_master->mutex)) {
    LOG_WARN("xSemaphoreGive");
  }

  return len;
}