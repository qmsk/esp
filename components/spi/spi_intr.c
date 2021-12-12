#include <spi_intr.h>
#include "spi_intr.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <rom/ets_sys.h>
#include <stdbool.h>

// init() called
static bool init = false;

static struct spi_intr_handler {
  spi_isr_t isr;
  void *arg;
} spi_intr_handler[SPI_INTR_MAX] = {
  [SPI_INTR_SPI0]   = {},
  [SPI_INTR_SPI1]   = {},
  [SPI_INTR_I2S]    = {},
};

static inline int spi_isr_call(enum spi_intr intr)
{
  struct spi_intr_handler *handler = &spi_intr_handler[intr];

  if (handler->isr) {
    handler->isr(handler->arg);

    return 1;
  } else {
    return 0;
  }
}

void IRAM_ATTR spi_isr(void *arg)
{
  uint32_t int_status = READ_PERI_REG(REG_SPI_INT_STATUS);

  LOG_ISR_DEBUG("int_status=%04x", int_status);

  if (int_status & SPI_INT_STATUS_SPI0) {
    if (!spi_isr_call(SPI_INTR_SPI0)) {
      // clear and disable
      spi0_intr_off();
    }
  }

  if (int_status & SPI_INT_STATUS_SPI1) {
    if (!spi_isr_call(SPI_INTR_SPI1)) {
      // clear and disable
      spi1_intr_off();
    }
  }

  if (int_status & SPI_INT_STATUS_I2S) {
    if (!spi_isr_call(SPI_INTR_I2S)) {
      // clear and disable
      i2s_intr_off();
    }
  }
}

void spi_intr_init()
{
  if (init) {
    return;
  }

  _xt_isr_attach(ETS_SPI_INUM, spi_isr, NULL);
  _xt_isr_unmask(1 << ETS_SPI_INUM);

  init = true;
}

void spi_intr_install(enum spi_intr intr, spi_isr_t isr, void *arg)
{
  taskENTER_CRITICAL();

  spi_intr_handler[intr] = (struct spi_intr_handler) { isr, arg };

  taskEXIT_CRITICAL();
}
