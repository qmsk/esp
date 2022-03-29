#include "../i2s_out.h"
#include "i2s.h"

#include <logging.h>
#include <spi_intr.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static void IRAM_ATTR i2s_isr(void *arg)
{
  struct i2s_out *i2s_out = arg;
  BaseType_t task_woken = 0;

  LOG_ISR_DEBUG("int_st=%08x", I2S0.int_st.val);

  if (I2S0.int_st.tx_put_data) {
    I2S0.int_ena.tx_put_data = 0;
  }

  if (I2S0.int_st.tx_wfull) {
    I2S0.int_ena.tx_wfull = 0;
  }

  if (I2S0.int_st.tx_rempty) {
    // unblock flush() task
    xEventGroupSetBitsFromISR(i2s_out->event_group, I2S_OUT_EVENT_GROUP_BIT_I2S_EOF, &task_woken);

    // interrupt will fire until disabled
    I2S0.int_ena.tx_rempty = 0;
  }

  i2s_intr_clear(&I2S0);

  if (task_woken) {
    portYIELD_FROM_ISR();
  }
}

int i2s_out_i2s_init(struct i2s_out *i2s_out)
{
  i2s_intr_disable(&I2S0);

  spi_intr_init();
  spi_intr_install(SPI_INTR_I2S, i2s_isr, i2s_out);

  return 0;
}

int i2s_out_i2s_setup(struct i2s_out *i2s_out, struct i2s_out_options options)
{
  taskENTER_CRITICAL();

  i2s_stop(&I2S0);
  i2s_reset(&I2S0);
  i2s_intr_clear(&I2S0);
  i2s_intr_disable(&I2S0);
  i2s_fifo_dma_disable(&I2S0);

  I2S0.conf.tx_slave_mod = 0; // generate output clock
  I2S0.conf.rx_slave_mod = 0;
  I2S0.conf.right_first = 0; // TX LEFT, RIGHT
  I2S0.conf.msb_right = 0; // FIFO is LEFT, RIGHT
  I2S0.conf.tx_msb_shift = 0;
  I2S0.conf.rx_msb_shift = 0;
  I2S0.conf.bits_mod = 0; // 16
  I2S0.conf.clkm_div_num = options.clock.clkm_div;
  I2S0.conf.bck_div_num = options.clock.bck_div;

  I2S0.fifo_conf.tx_fifo_mod = I2S_TX_FIFO_MODE_16BIT_FULL; // 16-bit left + 16-bit right
  I2S0.fifo_conf.rx_fifo_mod = I2S_RX_FIFO_MODE_16BIT_FULL; // unused

  I2S0.conf_chan.tx_chan_mod = I2S_TX_CHAN_MODE_DUAL; // transmit both channels
  I2S0.conf_chan.rx_chan_mod = I2S_RX_CHAN_MODE_DUAL; // transmit both channels

  // use DMA
  i2s_fifo_dma_enable(&I2S0);

  taskEXIT_CRITICAL();

  // reset eof state
  xEventGroupClearBits(i2s_out->event_group, I2S_OUT_EVENT_GROUP_BIT_I2S_EOF);

  return 0;
}

void i2s_out_i2s_start(struct i2s_out *i2s_out)
{
  taskENTER_CRITICAL();

  i2s_start_tx(&I2S0);

  taskEXIT_CRITICAL();
}

int i2s_out_i2s_flush(struct i2s_out *i2s_out)
{
  EventBits_t bits;

  if ((bits = xEventGroupGetBits(i2s_out->event_group)) & I2S_OUT_EVENT_GROUP_BIT_I2S_EOF) {
    // TX_REMPTY has already occured
    LOG_DEBUG("skip event_group bits=%08x", bits);
  } else {
    taskENTER_CRITICAL();

    i2s_intr_clear(&I2S0);
    i2s_intr_enable_tx(&I2S0);

    taskEXIT_CRITICAL();

    LOG_DEBUG("wait event_group bits=%08x", I2S_OUT_EVENT_GROUP_BIT_I2S_EOF);

    xEventGroupWaitBits(i2s_out->event_group, I2S_OUT_EVENT_GROUP_BIT_I2S_EOF, false, false, portMAX_DELAY);
  }

  return 0;
}

void i2s_out_i2s_stop(struct i2s_out *i2s_out)
{
  taskENTER_CRITICAL();

  i2s_stop_tx(&I2S0);

  taskEXIT_CRITICAL();
}
