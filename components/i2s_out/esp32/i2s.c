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

  i2s_ll_tx_stop(i2s_out->dev);

  i2s_ll_tx_reset(i2s_out->dev);
  i2s_ll_tx_reset_fifo(i2s_out->dev);

  i2s_intr_disable(i2s_out->dev, I2S_TX_REMPTY_INT_ENA);
  i2s_intr_clear(i2s_out->dev, I2S_TX_REMPTY_INT_CLR);

  i2s_ll_tx_set_slave_mod(i2s_out->dev, false);

  // TODO: SOC_I2S_SUPPORTS_PDM_RX/TX, SOC_I2S_SUPPORTS_TDM?
  i2s_ll_tx_enable_pdm(i2s_out->dev, false);

  // configure 16-bit word ordering, write(0x76543210) -> FIFO 0x10, 0x32, 0x54, 0x76 -> TX 76543210
  i2s_ll_tx_enable_msb_right(i2s_out->dev, false);
  i2s_ll_tx_enable_right_first(i2s_out->dev, false);

  // use tx_fifo_mod=0, tx_chan_mod=0
  i2s_ll_tx_force_enable_fifo_mod(i2s_out->dev, true);
  i2s_ll_tx_set_sample_bit(i2s_out->dev, I2S_BITS_PER_SAMPLE_16BIT, I2S_BITS_PER_CHAN_16BIT);
  i2s_ll_tx_enable_mono_mode(i2s_out->dev, false);

  i2s_ll_tx_enable_msb_shift(i2s_out->dev, false);
  i2s_ll_tx_set_ws_width(i2s_out->dev, 16);

  // likely to trigger in between DMA transfers, and stop the packet short?
  i2s_ll_tx_stop_on_fifo_empty(i2s_out->dev, false);

  // do not compress data
  i2s_ll_tx_bypass_pcm(i2s_out->dev, true);

  // using 160MHz I2S_CLK_D2CLK
  i2s_ll_mclk_div_t mclk_set = { .mclk_div = options.clock.clkm_div, .b = 0, .a = 1 };

  i2s_ll_tx_clk_set_src(i2s_out->dev, I2S_CLK_D2CLK);
  i2s_ll_tx_set_clk(i2s_out->dev, &mclk_set);
  i2s_ll_tx_set_bck_div_num(i2s_out->dev, options.clock.bck_div);

  taskEXIT_CRITICAL(&i2s_out->mux);

  LOG_DEBUG("conf=%08x fifo_conf=%08x conf_chan=%08x conf1=%08x conf2=%08x clkm_conf=%08x sample_rate_conf=%08x",
    i2s_out->dev->conf.val,
    i2s_out->dev->fifo_conf.val,
    i2s_out->dev->conf_chan.val,
    i2s_out->dev->conf1.val,
    i2s_out->dev->conf2.val,
    i2s_out->dev->clkm_conf.val,
    i2s_out->dev->sample_rate_conf.val
  );

  // reset eof state
  xEventGroupClearBits(i2s_out->event_group, I2S_OUT_EVENT_GROUP_BIT_I2S_EOF);

  return 0;
}

void i2s_out_i2s_start(struct i2s_out *i2s_out)
{
  LOG_DEBUG("");

  taskENTER_CRITICAL(&i2s_out->mux);

  // NOTE: there seems to always be three extra BCK cycles at the start of TX
  // XXX: occasionally, some trailing bits of the last TX will get transmitted first...
  //      possibly if DMA is slow, and the FIFO does not yet contain the new data?
  //      let's hope that the EOF frame is always zeroes, and zero bytes at the start are harmless...
  i2s_ll_tx_start(i2s_out->dev);

  taskEXIT_CRITICAL(&i2s_out->mux);
}

int i2s_out_i2s_flush(struct i2s_out *i2s_out)
{
  EventBits_t bits;

  if ((bits = xEventGroupGetBits(i2s_out->event_group)) & I2S_OUT_EVENT_GROUP_BIT_I2S_EOF) {
    // TX_REMPTY has already occured
    LOG_DEBUG("skip event_group bits=%08x", bits);
  } else {
    taskENTER_CRITICAL(&i2s_out->mux);

    i2s_intr_clear(i2s_out->dev, I2S_TX_REMPTY_INT_CLR);
    i2s_intr_enable(i2s_out->dev, I2S_TX_REMPTY_INT_ENA);

    taskEXIT_CRITICAL(&i2s_out->mux);

    LOG_DEBUG("wait event_group bits=%08x", I2S_OUT_EVENT_GROUP_BIT_I2S_EOF);

    xEventGroupWaitBits(i2s_out->event_group, I2S_OUT_EVENT_GROUP_BIT_I2S_EOF, false, false, portMAX_DELAY);
  }

  return 0;
}

void i2s_out_i2s_stop(struct i2s_out *i2s_out)
{
  LOG_DEBUG("");

  taskENTER_CRITICAL(&i2s_out->mux);

  i2s_ll_tx_stop(i2s_out->dev);

  taskEXIT_CRITICAL(&i2s_out->mux);
}
