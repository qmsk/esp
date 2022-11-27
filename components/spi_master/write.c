#include <spi_master.h>
#include "spi_master.h"
#include "spi_dev.h"
#include <logging.h>

// shrink size to aligment
#define TRUNC(size, align) ((size) & ~((align) - 1))

static inline void spi_master_mosi(struct spi_master *spi_master, uint32_t *data, unsigned count)
{
  LOG_DEBUG("spi_master=%p data=%p count=%u", spi_master, data, count);

  SPI_DEV.user1.usr_mosi_bitlen = count * 32 - 1;

  for (unsigned i = 0; i < count && i < 16; i++) {
    SPI_DEV.data_buf[i] = data[i];
  }
}

static inline void spi_master_start(struct spi_master *spi_master)
{
  LOG_DEBUG("spi_master=%p", spi_master);

  SPI_DEV.cmd.usr = 1;
}

static int spi_master_wait_trans_done(struct spi_master *spi_master, TickType_t block_time)
{
  int err;

  while (SPI_DEV.cmd.usr) {
    LOG_DEBUG("spi_master=%p: busy", spi_master);

    if ((err = spi_master_intr_wait_trans_done(spi_master, block_time))) {
      LOG_ERROR("spi_master_intr_wait_trans_done");
      return err;
    }

    // loop again
  }

  LOG_DEBUG("spi_master=%p: idle", spi_master);

  return 0;
}

int spi_master_open(struct spi_master *spi_master, struct spi_write_options options)
{
  int err;

  if (!xSemaphoreTake(spi_master->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  if ((err = spi_master_wait_trans_done(spi_master, portMAX_DELAY))) {
    LOG_ERROR("spi_master_wait_trans_done");
    goto error;
  }

  // reconfigure
  if (options.mode && (options.mode & SPI_MODE_FLAGS) != spi_master->mode) {
    LOG_DEBUG("set mode=%02x", options.mode);

    if ((err = spi_master_mode(spi_master, options.mode))) {
      LOG_ERROR("spi_master_mode");
      goto error;
    }
  }

  if (options.clock && options.clock != spi_master->clock) {
    LOG_DEBUG("set clock=%d", options.clock);

    if ((err = spi_master_clock(spi_master, options.clock))) {
      LOG_ERROR("spi_master_clock");
      goto error;
    }
  }

  return 0;

error:
  if (!xSemaphoreGive(spi_master->mutex)) {
    LOG_WARN("xSemaphoreGive");
  }

  return err;
}

int spi_master_write(struct spi_master *spi_master, void *data, size_t len)
{
  int err;

  LOG_DEBUG("spi_master=%p data=%p len=%u", spi_master, data, len);

  if (len > SPI_WRITE_MAX) {
    len = SPI_WRITE_MAX;
  } else {
    len = TRUNC(len, sizeof(uint32_t));
  }

  // wait
  if ((err = spi_master_wait_trans_done(spi_master, portMAX_DELAY))) {
    LOG_ERROR("spi_master_wait_trans_done");
    return err;
  }

  // usr command structure: mosi only
  SPI_DEV.user.usr_command = 0;
  SPI_DEV.user.usr_addr = 0;
  SPI_DEV.user.usr_dummy = 0;
  SPI_DEV.user.usr_mosi = 1;
  SPI_DEV.user.usr_miso = 0;

  // TODO: assumes uint32_t alignment, support non-aligned?
  spi_master_mosi(spi_master, data, len / 4);

  spi_master_start(spi_master);

  return len;
}

int spi_master_flush(struct spi_master *spi_master)
{
  return spi_master_wait_trans_done(spi_master, portMAX_DELAY);
}

int spi_master_close(struct spi_master *spi_master)
{
  if (!xSemaphoreGive(spi_master->mutex)) {
    LOG_WARN("xSemaphoreGive");
    return -1;
  }

  return 0;
}
