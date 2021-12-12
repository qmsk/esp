#pragma once

#include <esp8266/i2s_struct.h>
#include <esp8266/i2s_register.h>

#include <esp8266/eagle_soc.h>
#include <esp8266/pin_mux_register.h>

// glitchless version of PIN_FUNC_SELECT(), does not reset pin func to default if already set
#define IDEMPOTENT_PIN_FUNC_SELECT(PIN_NAME, FUNC) \
  SET_PERI_REG_BITS(PIN_NAME, PERIPHS_IO_MUX_FUNC, ((FUNC & 0x4) << 2) | (FUNC & 0x3), PERIPHS_IO_MUX_FUNC_S)

#define I2S_INT_CLR_BITS (I2S_I2S_TX_REMPTY_INT_CLR | I2S_I2S_TX_WFULL_INT_CLR | I2S_I2S_RX_REMPTY_INT_CLR | I2S_I2S_RX_WFULL_INT_CLR | I2S_I2S_PUT_DATA_INT_CLR | I2S_I2S_TAKE_DATA_INT_CLR)

enum i2s_tx_fifo_mode {
  I2S_TX_FIFO_MODE_16BIT_FULL  = 0,  // 16-bit + 16-bit
  I2S_TX_FIFO_MODE_16BIT_HALF  = 1,  // 16-bit / 16-bit
  I2S_TX_FIFO_MODE_24BIT_FULL  = 2,  // 24-bit / 8-bit + 24-bit / 8-bit
  I2S_TX_FIFO_MODE_24BIT_HALF  = 3,  // 24-bit / 8-bit
};

enum i2s_rx_fifo_mode {
  I2S_RX_FIFO_MODE_16BIT_FULL  = 0,  // 16-bit + 16-bit
  I2S_RX_FIFO_MODE_16BIT_HALF  = 1,  // 16-bit / 16-bit
  I2S_RX_FIFO_MODE_24BIT_FULL  = 2,  // 24-bit / 8-bit + 24-bit / 8-bit
  I2S_RX_FIFO_MODE_24BIT_HALF  = 3,  // 24-bit / 8-bit
};

enum i2s_tx_chan_mode {
  I2S_TX_CHAN_MODE_DUAL       = 0,
  I2S_TX_CHAN_MODE_RIGHT      = 1,
  I2S_TX_CHAN_MODE_LEFT       = 2,
};

enum i2s_rx_chan_mode {
  I2S_RX_CHAN_MODE_DUAL       = 0,
  I2S_RX_CHAN_MODE_RIGHT      = 1,
  I2S_RX_CHAN_MODE_LEFT       = 2,
};

static inline void i2s_stop(volatile i2s_struct_t *i2s)
{
  i2s->conf.val &= ~(I2S_I2S_TX_START | I2S_I2S_RX_START);
}

static inline void i2s_start(volatile i2s_struct_t *i2s)
{
  i2s->conf.val |= (I2S_I2S_TX_START | I2S_I2S_RX_START);
}

static inline void i2s_start_tx(volatile i2s_struct_t *i2s)
{
  i2s->conf.tx_start = 1;
}
static inline void i2s_start_rx(volatile i2s_struct_t *i2s)
{
  i2s->conf.rx_start = 1;
}

static inline void i2s_reset(volatile i2s_struct_t *i2s)
{
  i2s->conf.val |= I2S_I2S_RESET_MASK;
  i2s->conf.val &= ~I2S_I2S_RESET_MASK;
}

static inline void i2s_intr_disable(volatile i2s_struct_t *i2s)
{
  i2s->int_ena.val = 0;
}

static inline void i2s_intr_disable_tx(volatile i2s_struct_t *i2s)
{
  i2s->int_ena.tx_rempty = 0;
}

static inline void i2s_intr_enable_tx(volatile i2s_struct_t *i2s)
{
  i2s->int_ena.tx_put_data = 1;
  i2s->int_ena.tx_wfull = 1;
  i2s->int_ena.tx_rempty = 1;
}

static inline void i2s_intr_clear(volatile i2s_struct_t *i2s)
{
  i2s->int_clr.val |= I2S_INT_CLR_BITS;
}

static inline void i2s_fifo_dma_disable(volatile i2s_struct_t *i2s)
{
  i2s->fifo_conf.dscr_en = 0;
}

static inline void i2s_fifo_dma_enable(volatile i2s_struct_t *i2s)
{
  i2s->fifo_conf.dscr_en = 1;
}
