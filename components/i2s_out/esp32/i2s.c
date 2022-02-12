#include "../i2s_out.h"
#include "intr.h"

#include <hal/i2s_ll.h>

#include <logging.h>

int i2s_out_i2s_init(struct i2s_out *i2s_out)
{
  return 0;
}

int i2s_out_i2s_setup(struct i2s_out *i2s_out, struct i2s_out_options options)
{
  LOG_DEBUG("clock(clkm=1/%d bck=1/%d)", options.clock.clkm_div, options.clock.bck_div);

  taskENTER_CRITICAL(&i2s_out->mux);

  i2s_ll_enable_clock(i2s_out->dev);

  i2s_ll_tx_stop(i2s_out->dev);
  i2s_ll_tx_reset(i2s_out->dev);

  i2s_intr_disable(i2s_out->dev, I2S_TX_REMPTY_INT_ENA);

  i2s_ll_enable_dma(i2s_out->dev, false);

  i2s_ll_tx_set_slave_mod(i2s_out->dev, false);

  // TODO: SOC_I2S_SUPPORTS_PDM_RX/TX, SOC_I2S_SUPPORTS_TDM?
  i2s_ll_tx_enable_pdm(i2s_out->dev, false);

  // XXX: bit ordering?
  i2s_ll_tx_enable_msb_right(i2s_out->dev, true);
  i2s_ll_tx_enable_right_first(i2s_out->dev, false);  // TX channels left -> right, matching FIFO byte order
  i2s_ll_tx_force_enable_fifo_mod(i2s_out->dev, true);

  // use tx_fifo_mod=0, tx_chan_mod=0
  i2s_ll_tx_set_sample_bit(i2s_out->dev, I2S_BITS_PER_SAMPLE_16BIT, I2S_BITS_PER_CHAN_16BIT);
  i2s_ll_tx_enable_mono_mode(i2s_out->dev, false);

  i2s_ll_tx_enable_msb_shift(i2s_out->dev, false);
  i2s_ll_tx_set_ws_width(i2s_out->dev, 16);

  // using 160MHz I2S_CLK_D2CLK
  i2s_ll_mclk_div_t mclk_set = { .mclk_div = options.clock.clkm_div, .b = 0, .a = 1 };

  i2s_ll_tx_clk_set_src(i2s_out->dev, I2S_CLK_D2CLK);
  i2s_ll_tx_set_clk(i2s_out->dev, &mclk_set);
  i2s_ll_tx_set_bck_div_num(i2s_out->dev, options.clock.bck_div);

  taskEXIT_CRITICAL(&i2s_out->mux);

  return 0;
}

void i2s_out_i2s_start(struct i2s_out *i2s_out)
{
  LOG_DEBUG("");

  taskENTER_CRITICAL(&i2s_out->mux);

  i2s_ll_tx_start(i2s_out->dev);

  taskEXIT_CRITICAL(&i2s_out->mux);
}

int i2s_out_i2s_flush(struct i2s_out *i2s_out)
{
  LOG_DEBUG("set i2s_flush_task=%p", xTaskGetCurrentTaskHandle());

  taskENTER_CRITICAL(&i2s_out->mux);

  i2s_out->i2s_flush_task = xTaskGetCurrentTaskHandle();

  i2s_intr_enable(i2s_out->dev, I2S_TX_REMPTY_INT_ENA);

  taskEXIT_CRITICAL(&i2s_out->mux);

  LOG_DEBUG("wait i2s_flush_task=%p", i2s_out->i2s_flush_task);

  // wait for tx to complete and break to start
  if (!ulTaskNotifyTake(true, portMAX_DELAY)) {
    LOG_WARN("ulTaskNotifyTake: timeout");
    return -1;
  }

  LOG_DEBUG("wait done");

  return 0;
}
