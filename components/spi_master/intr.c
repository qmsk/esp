#include "spi_master.h"
#include "spi_dev.h"

#include <logging.h>

#include <rom/ets_sys.h>

#define SPI_INT_STATUS (SPI_INT_STATUS_SPI1)

static inline void spi0_interrupt_clear()
{
  // XXX: clear, based on driver/spi.c example
  SPI0.slave.val &= ~0x3ff;
}

static inline void spi1_interrupt_clear()
{
  // XXX: clear, based on driver/spi.c example
  while (SPI1.slave.val & 0x1f) {
    SPI1.slave.val &= ~0x1f;
  }
}

static IRAM_ATTR void spi_master_interrupt(void *arg)
{
  struct spi_master *spi_master = arg;
  BaseType_t higher_task_woken;

  if (READ_PERI_REG(REG_SPI_INT_STATUS) & SPI_INT_STATUS_SPI0) {
    // clear
    spi0_interrupt_clear();
  }

  if (READ_PERI_REG(REG_SPI_INT_STATUS) & SPI_INT_STATUS_SPI1) {
    if (SPI_DEV.slave.trans_done) {
      xSemaphoreGiveFromISR(spi_master->trans_done, &higher_task_woken);
    }

    spi1_interrupt_clear();
  }

  portEND_SWITCHING_ISR(higher_task_woken);
}

static inline void spi_master_interrupt_disable(struct spi_master *spi_master)
{
  LOG_DEBUG("spi_master=%p", spi_master);

  SPI_DEV.slave.val &= ~0x1f;
}

static inline void spi_master_interrupt_enable_trans(struct spi_master *spi_master)
{
  LOG_DEBUG("spi_master=%p", spi_master);

  SPI_DEV.slave.trans_inten = 1;
}

int spi_master_interrupt_wait_trans(struct spi_master *spi_master, TickType_t block_time)
{
  while (SPI_DEV.cmd.usr) {
    LOG_DEBUG("spi_master=%p: busy", spi_master);

    spi_master_interrupt_enable_trans(spi_master);

    if (!xSemaphoreTake(spi_master->trans_done, block_time)) {
      LOG_WARN("xSemaphoreTake");
      return 1;
    }

    // loop again
  }

  LOG_DEBUG("spi_master=%p: idle", spi_master);

  return 0;
}

int spi_master_interrupt_init(struct spi_master *spi_master)
{
  if (!(spi_master->trans_done = xSemaphoreCreateBinary())) {
    LOG_ERROR("xSemaphoreCreateBinary");
    return -1;
  }

  spi_master_interrupt_disable(spi_master);

  _xt_isr_attach(ETS_SPI_INUM, spi_master_interrupt, spi_master);
  _xt_isr_unmask(1 << ETS_SPI_INUM);

  return 0;
}
