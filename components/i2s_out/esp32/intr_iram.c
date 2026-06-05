#define ERROR

#include "../i2s_out.h"
#include "dma.h"
#include "intr.h"

#include <logging.h>

// we may miss some EOF ISR for intermediate DMA descriptors, ensure we unblock i2s_out_dma_wait() on owner up to eof_desc
static void i2s_out_intr_dma_out_desc(struct i2s_out *i2s_out, struct dma_desc *eof_desc, BaseType_t *pxHigherPriorityTaskWoken)
{
  // update dma_out_desc and reset descs for write
  for (struct dma_desc *desc = i2s_out->dma_out_desc; (desc != NULL) && (desc != eof_desc); desc = desc->next) {
    LOG_ISR_WARN("missed desc=%p owner=%u len=%u", desc, desc->owner, desc->len);

    i2s_dma_desc_reset(desc);
  }

  i2s_dma_desc_reset(eof_desc);

  i2s_out->dma_out_desc = eof_desc->next;
  
  // unblock wait()
  if (i2s_out->dma_out_task) {
    xTaskNotifyFromISR(i2s_out->dma_out_task, I2S_OUT_TASK_NOTIFY_BIT_DMA_OUT, eSetBits, pxHigherPriorityTaskWoken);

    i2s_out->dma_out_task = NULL;
  }
}

static void i2s_out_intr_dma_done(struct i2s_out *i2s_out, BaseType_t *pxHigherPriorityTaskWoken)
{
  // unblock flush()
  i2s_out->dma_done = true;

  if (i2s_out->dma_done_task) {
      xTaskNotifyFromISR(i2s_out->dma_done_task, I2S_OUT_TASK_NOTIFY_BIT_DMA_DONE, eSetBits, pxHigherPriorityTaskWoken);

      i2s_out->dma_done_task = NULL;
  }
}

static void i2s_out_intr_i2s_done(struct i2s_out *i2s_out, BaseType_t *pxHigherPriorityTaskWoken)
{
  // unblock flush() task
  i2s_out->i2s_done = true;

  if (i2s_out->i2s_done_task) {
    xTaskNotifyFromISR(i2s_out->i2s_done_task, I2S_OUT_TASK_NOTIFY_BIT_I2S_DONE, eSetBits, pxHigherPriorityTaskWoken);

    i2s_out->i2s_done_task = NULL;
  }
}

static void i2s_intr_out_dscr_err_handler(struct i2s_out *i2s_out)
{
  uint32_t dscr_addr;

  i2s_intr_clear(i2s_out->dev, I2S_OUT_DSCR_ERR_INT_CLR);
  i2s_dma_tx_get_des_addr(i2s_out->dev, &dscr_addr);

  LOG_ISR_ERROR("desc=%p", dscr_addr);
}

static void i2s_intr_out_eof_handler(struct i2s_out *i2s_out, BaseType_t *pxHigherPriorityTaskWoken)
{
  uint32_t eof_addr;

  i2s_intr_clear(i2s_out->dev, I2S_OUT_EOF_INT_CLR);
  i2s_ll_tx_get_eof_des_addr(i2s_out->dev, &eof_addr);

  struct dma_desc *eof_desc = (struct dma_desc *) eof_addr;

  if (eof_desc >= i2s_out->dma_data_desc && eof_desc < i2s_out->dma_data_desc + i2s_out->dma_data_count) {
    LOG_ISR_DEBUG("data desc=%p owner=%u len=%u", eof_desc, eof_desc->owner, eof_desc->len);

    i2s_out_intr_dma_out_desc(i2s_out, eof_desc, pxHigherPriorityTaskWoken);

  } else if (i2s_out->dma_repeat_desc && eof_desc >= i2s_out->dma_repeat_desc && eof_desc < i2s_out->dma_repeat_desc + i2s_out->dma_data_count * i2s_out->dma_repeat_count) {
    LOG_ISR_DEBUG("repeat desc=%p owner=%u len=%u", eof_desc, eof_desc->owner, eof_desc->len);

    i2s_out_intr_dma_out_desc(i2s_out, eof_desc, pxHigherPriorityTaskWoken);

  } else if (eof_desc == i2s_out->dma_end_desc) {
    LOG_ISR_DEBUG("end desc=%p owner=%u len=%u, dma_write_desc=%p dma_out_desc=%p", eof_desc, eof_desc->owner, eof_desc->len, i2s_out->dma_write_desc, i2s_out->dma_out_desc);
    
    // we may miss some EOF ISR for intermediate DMA descriptors, unblock i2s_out_dma_wait()
    i2s_out_intr_dma_out_desc(i2s_out, eof_desc, pxHigherPriorityTaskWoken);
    i2s_out_intr_dma_done(i2s_out, pxHigherPriorityTaskWoken);

    // XXX: ensure tx rempty intr is enabled, in case it fired during DMA and was disabled?
    i2s_intr_clear(i2s_out->dev, I2S_TX_REMPTY_INT_CLR);
    i2s_intr_enable(i2s_out->dev, I2S_TX_REMPTY_INT_ENA);

  } else {
    LOG_ISR_ERROR("unknown desc=%p owner=%u len=%u", eof_desc, eof_desc->owner, eof_desc->len);
  }
}

static void i2s_intr_tx_rempty_handler(struct i2s_out *i2s_out, BaseType_t *pxHigherPriorityTaskWoken)
{
  LOG_ISR_DEBUG("");

  // interrupt will fire until disabled
  i2s_intr_disable(i2s_out->dev, I2S_TX_REMPTY_INT_ENA);
  i2s_intr_clear(i2s_out->dev, I2S_TX_REMPTY_INT_CLR);

  if (!i2s_out->dma_done) {
    // XXX: ignore if fired before dma_done, will be re-enabled
    // XXX: may indicate a timing glitch in the output data?
    LOG_ISR_ERROR("tx rempty dma_done=%u i2s_done=%u", i2s_out->dma_done, i2s_out->i2s_done);
  } else {
    LOG_ISR_DEBUG("tx rempty dma_done=%u i2s_done=%u", i2s_out->dma_done, i2s_out->i2s_done);

    i2s_out_intr_i2s_done(i2s_out, pxHigherPriorityTaskWoken);
  }
}

void i2s_intr_handler(void *arg)
{
  struct i2s_out *i2s_out = arg;
  uint32_t int_st = i2s_ll_get_intr_status(i2s_out->dev);
  BaseType_t pxHigherPriorityTaskWoken = pdFALSE;

  if (!int_st) {
    return;
  }

  taskENTER_CRITICAL_ISR(&i2s_out->mux);

  if (int_st & I2S_OUT_DSCR_ERR_INT_ST) {
    i2s_intr_out_dscr_err_handler(i2s_out);
  }
  if (int_st & I2S_OUT_EOF_INT_ST) {
    i2s_intr_out_eof_handler(i2s_out, &pxHigherPriorityTaskWoken);
  }
  if (int_st & I2S_TX_REMPTY_INT_ST) {
    i2s_intr_tx_rempty_handler(i2s_out, &pxHigherPriorityTaskWoken);
  }

  taskEXIT_CRITICAL_ISR(&i2s_out->mux);

  if (pxHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}
