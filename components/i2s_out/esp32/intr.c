#include "../i2s_out.h"
#include "dma.h"
#include "intr.h"

#include <logging.h>

#include <esp_err.h>
#include <esp_intr_alloc.h>
#include <soc/soc.h>


// use a non-shared intr
#define I2S_INTR_ALLOC_FLAGS (0)

static const int i2s_irq[I2S_PORT_MAX] = {
  [I2S_PORT_0]  = ETS_I2S0_INTR_SOURCE,
  [I2S_PORT_1]  = ETS_I2S1_INTR_SOURCE,
};

void IRAM_ATTR i2s_intr_out_dscr_err_handler(struct i2s_out *i2s_out, BaseType_t *task_wokenp)
{
  uint32_t dscr_addr;

  i2s_dma_tx_get_des_addr(i2s_out->dev, &dscr_addr);

  LOG_ISR_WARN("desc=%p", dscr_addr);

  i2s_intr_clear(i2s_out->dev, I2S_OUT_DSCR_ERR_INT_CLR);
}

void IRAM_ATTR i2s_intr_out_eof_handler(struct i2s_out *i2s_out, BaseType_t *task_wokenp)
{
  uint32_t eof_addr;

  i2s_ll_tx_get_eof_des_addr(i2s_out->dev, &eof_addr);

  struct dma_desc *eof_desc = (struct dma_desc *) eof_addr;

  LOG_ISR_DEBUG("desc=%p", eof_desc);

  // NOTE: this is unlikely to stop DMA before this repeats at least once
  i2s_ll_tx_stop_link(i2s_out->dev);
  i2s_ll_rx_stop_link(i2s_out->dev);

  // unblock flush() task
  xEventGroupSetBitsFromISR(i2s_out->event_group, I2S_OUT_EVENT_GROUP_BIT_DMA_EOF, task_wokenp);

  i2s_intr_clear(i2s_out->dev, I2S_OUT_EOF_INT_CLR);
}

void IRAM_ATTR i2s_intr_tx_rempty_handler(struct i2s_out *i2s_out, BaseType_t *task_wokenp)
{
  LOG_ISR_DEBUG("");

  // unblock flush() task
  xEventGroupSetBitsFromISR(i2s_out->event_group, I2S_OUT_EVENT_GROUP_BIT_I2S_EOF, task_wokenp);

  // interrupt will fire until disabled
  i2s_intr_disable(i2s_out->dev, I2S_TX_REMPTY_INT_ENA);
  i2s_intr_clear(i2s_out->dev, I2S_TX_REMPTY_INT_CLR);
}

void IRAM_ATTR i2s_intr_handler(void *arg)
{
  struct i2s_out *i2s_out = arg;
  uint32_t int_st = i2s_ll_get_intr_status(i2s_out->dev);
  BaseType_t task_woken = pdFALSE;

  if (!int_st) {
    return;
  }

  taskENTER_CRITICAL_ISR(&i2s_out->mux);

  if (int_st & I2S_OUT_DSCR_ERR_INT_ST) {
    i2s_intr_out_dscr_err_handler(i2s_out, &task_woken);
  }
  if (int_st & I2S_OUT_EOF_INT_ST) {
    i2s_intr_out_eof_handler(i2s_out, &task_woken);
  }
  if (int_st & I2S_TX_REMPTY_INT_ST) {
    i2s_intr_tx_rempty_handler(i2s_out, &task_woken);
  }

  taskEXIT_CRITICAL_ISR(&i2s_out->mux);

  if (task_woken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

int i2s_out_intr_setup(struct i2s_out *i2s_out, const struct i2s_out_options *options)
{
  esp_err_t err = 0;

  LOG_DEBUG("intr=%p", i2s_out->intr);

  taskENTER_CRITICAL(&i2s_out->mux);

  i2s_intr_disable_all(i2s_out->dev);

  if (!i2s_out->intr) {
    err = esp_intr_alloc(i2s_irq[i2s_out->port], I2S_INTR_ALLOC_FLAGS, i2s_intr_handler, i2s_out, &i2s_out->intr);
  }

  taskEXIT_CRITICAL(&i2s_out->mux);

  if (err) {
    LOG_ERROR("esp_intr_alloc: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

void i2s_out_intr_teardown(struct i2s_out *i2s_out)
{
  esp_err_t err;

  LOG_DEBUG("intr=%p", i2s_out->intr);

  taskENTER_CRITICAL(&i2s_out->mux);

  err = esp_intr_free(i2s_out->intr);

  i2s_out->intr = NULL;

  taskEXIT_CRITICAL(&i2s_out->mux);

  if (err) {
    LOG_WARN("esp_intr_free: %s", esp_err_to_name(err));
  }
}
