#define ERROR 

#include "../i2s_out.h"
#include "dma.h"
#include "intr.h"

#include <logging.h>

EventBits_t i2s_intr_out_dscr_err_handler(struct i2s_out *i2s_out)
{
  uint32_t dscr_addr;

  i2s_intr_clear(i2s_out->dev, I2S_OUT_DSCR_ERR_INT_CLR);
  i2s_dma_tx_get_des_addr(i2s_out->dev, &dscr_addr);

  LOG_ISR_ERROR("desc=%p", dscr_addr);

  return 0;
}

EventBits_t i2s_intr_out_eof_handler(struct i2s_out *i2s_out, BaseType_t *pxHigherPriorityTaskWoken)
{
  uint32_t eof_addr;

  i2s_intr_clear(i2s_out->dev, I2S_OUT_EOF_INT_CLR);
  i2s_ll_tx_get_eof_des_addr(i2s_out->dev, &eof_addr);

  LOG_ISR_DEBUG("desc=%p", eof_addr);

  struct dma_desc *eof_desc = (struct dma_desc *) eof_addr;

  // only handle normal out_desc, not repeat_desc or end_desc
  if (eof_desc >= i2s_out->dma_out_desc && eof_desc < i2s_out->dma_out_desc + i2s_out->dma_out_count) {
    LOG_ISR_DEBUG("eof desc=%p owner=%u len=%u", eof_desc, eof_desc->owner, eof_desc->len);

    // we may miss some EOF ISR for intermediate DMA descriptors, unblock i2s_out_dma_wait()
    if (i2s_out->dma_eof_desc) {
      for (struct dma_desc *desc = eof_desc; desc > i2s_out->dma_eof_desc; desc--) {
        if (desc != eof_desc) {
          LOG_ISR_WARN("miss desc=%p owner=%u len=%u", desc, desc->owner, desc->len);
        }

        i2s_dma_desc_reset(desc);
      }
    } else {
      i2s_dma_desc_reset(eof_desc);
    }

    i2s_out->dma_eof_desc = eof_desc;

    // unblock get() task
    return I2S_OUT_EVENT_GROUP_BIT_DMA_EOF;

  } else if (i2s_out->dma_repeat_desc && eof_desc >= i2s_out->dma_repeat_desc && eof_desc < i2s_out->dma_repeat_desc + i2s_out->dma_out_count * i2s_out->dma_repeat_count) {
    LOG_ISR_DEBUG("ignore repeat desc=%p owner=%u len=%u", eof_desc, eof_desc->owner, eof_desc->len);

    return 0;

  } else if (eof_desc == i2s_out->dma_end_desc) {
    // we may miss some EOF ISR for intermediate DMA descriptors, unblock i2s_out_dma_wait()
    LOG_ISR_WARN("end desc=%p owner=%u len=%u, dma_write_desc=%p dma_eof_desc=%p", eof_desc, eof_desc->owner, eof_desc->len, i2s_out->dma_write_desc, i2s_out->dma_eof_desc);

    eof_desc = i2s_out->dma_out_desc + i2s_out->dma_out_count - 1;
    
    for (struct dma_desc *desc = eof_desc; desc > i2s_out->dma_eof_desc; desc--) {
      i2s_dma_desc_reset(desc);
    }

    // unblock wait()
    i2s_out->dma_eof_desc = eof_desc;

    if (i2s_out->dma_eof_task) {
        xTaskNotifyFromISR(i2s_out->dma_eof_task, I2S_OUT_TASK_NOTIFY_BIT_DMA_EOF, eSetBits, pxHigherPriorityTaskWoken);

        i2s_out->dma_eof_task = NULL;
    }

    // unblock flush()
    i2s_out->dma_done = true;

    if (i2s_out->dma_done_task) {
        xTaskNotifyFromISR(i2s_out->dma_done_task, I2S_OUT_TASK_NOTIFY_BIT_DMA_DONE, eSetBits, pxHigherPriorityTaskWoken);

        i2s_out->dma_done_task = NULL;
    }

    // unblock get(), flush() task
    return I2S_OUT_EVENT_GROUP_BIT_DMA_EOF | I2S_OUT_EVENT_GROUP_BIT_DMA_DONE;

  } else {
    LOG_ISR_ERROR("ignore desc=%p owner=%u len=%u", eof_desc, eof_desc->owner, eof_desc->len);
    
    return 0;
  }
}

EventBits_t i2s_intr_tx_rempty_handler(struct i2s_out *i2s_out)
{
  LOG_ISR_DEBUG("");

  // interrupt will fire until disabled
  i2s_intr_disable(i2s_out->dev, I2S_TX_REMPTY_INT_ENA);
  i2s_intr_clear(i2s_out->dev, I2S_TX_REMPTY_INT_CLR);

  return I2S_OUT_EVENT_GROUP_BIT_I2S_EOF;
}

void i2s_intr_handler(void *arg)
{
  struct i2s_out *i2s_out = arg;
  uint32_t int_st = i2s_ll_get_intr_status(i2s_out->dev);
  EventBits_t event_bits = 0;
  BaseType_t pxHigherPriorityTaskWoken = pdFALSE;

  if (!int_st) {
    return;
  }

  taskENTER_CRITICAL_ISR(&i2s_out->mux);

  if (int_st & I2S_OUT_DSCR_ERR_INT_ST) {
    event_bits |= i2s_intr_out_dscr_err_handler(i2s_out);
  }
  if (int_st & I2S_OUT_EOF_INT_ST) {
    event_bits |= i2s_intr_out_eof_handler(i2s_out, &pxHigherPriorityTaskWoken);
  }
  if (int_st & I2S_TX_REMPTY_INT_ST) {
    event_bits |= i2s_intr_tx_rempty_handler(i2s_out);
  }

  if (event_bits) {
    // XXX: loops via the timer daemon task, replace with a semaphore?
    if (xEventGroupSetBitsFromISR(i2s_out->event_group, event_bits, &pxHigherPriorityTaskWoken) == pdFALSE) {
      LOG_ISR_ERROR("xEventGroupSetBitsFromISR");
    }
  }

  taskEXIT_CRITICAL_ISR(&i2s_out->mux);

  if (pxHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}
