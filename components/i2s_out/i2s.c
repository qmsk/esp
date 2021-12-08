#include "i2s_out.h"
#include "i2s.h"

#include <logging.h>

#include <esp8266/pin_mux_register.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define I2S I2S0

void i2s_out_i2s_setup(struct i2s_out *i2s_out, struct i2s_out_options options)
{
  taskENTER_CRITICAL();

  i2s_stop(&I2S0);
  i2s_reset(&I2S0);
  i2s_intr_clear(&I2S0);
  i2s_intr_disable(&I2S0);
  i2s_fifo_dma_disable(&I2S0);

  I2S0.conf.tx_slave_mod = 0; // generate output clock
  I2S0.conf.rx_slave_mod = 0;
  I2S0.conf.right_first = 1; // XXX: ???
  I2S0.conf.msb_right = 1; // XXX: ???
  // XXX: tx_msb_shift?
  // XXX: rx_msb_shift?
  I2S0.conf.bits_mod = 0; // XXX: ???
  I2S0.conf.clkm_div_num = options.clock.clkm_div;
  I2S0.conf.bck_div_num = options.clock.bck_div;

  I2S0.fifo_conf.tx_fifo_mod = I2S_TX_FIFO_MODE_16BIT_FULL; // transmit 2 channels of 16-bit samples per 32-bit FIFO word
  I2S0.fifo_conf.rx_fifo_mod = I2S_RX_FIFO_MODE_16BIT_FULL; // unused

  I2S0.conf_chan.tx_chan_mod = I2S_TX_CHAN_MODE_DUAL; // transmit both channels
  I2S0.conf_chan.rx_chan_mod = I2S_RX_CHAN_MODE_DUAL; // transmit both channels

  // use DMA
  i2s_fifo_dma_enable(&I2S0);

  // configure data out pin
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);

  taskEXIT_CRITICAL();
}

void i2s_out_i2s_start(struct i2s_out *i2s_out)
{
  taskENTER_CRITICAL();

  i2s_start_tx(&I2S0);

  taskEXIT_CRITICAL();
}
