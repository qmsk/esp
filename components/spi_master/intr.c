#include "spi_master.h"
#include "spi_dev.h"

#include <logging.h>
#include <spi_intr.h>

#include <rom/ets_sys.h>

static inline void spi1_intr_disable()
{
  SPI1.slave.val &= ~0x1f;
}

static inline void spi1_intr_enable_trans()
{
  SPI1.slave.trans_inten = 1;
}

static inline void spi1_interrupt_clear()
{
  // XXX: clear, based on driver/spi.c example
  while (SPI1.slave.val & 0x1f) {
    SPI1.slave.val &= ~0x1f;
  }
}

static IRAM_ATTR void spi1_interrupt(void *arg)
{
  struct spi_master *spi_master = arg;
  BaseType_t higher_task_woken;

  if (SPI1.slave.trans_done) {
    xSemaphoreGiveFromISR(spi_master->trans_done, &higher_task_woken);
  }

  spi1_interrupt_clear();

  if (higher_task_woken) {
    portYIELD_FROM_ISR();
  }
}

int spi_master_intr_init(struct spi_master *spi_master)
{
  spi1_intr_disable();

  spi_intr_init();
  spi_intr_install(SPI_INTR_SPI1, spi1_interrupt, spi_master);

  return 0;
}

int spi_master_intr_wait_trans_done(struct spi_master *spi_master, TickType_t block_time)
{
  spi1_intr_enable_trans(spi_master);

  if (!xSemaphoreTake(spi_master->trans_done, block_time)) {
    LOG_WARN("xSemaphoreTake");
    return 1;
  }

  return 0;
}
