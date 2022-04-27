#pragma once

#include <hal/i2s_ll.h>

static inline void i2s_out_tx_set_fifo_mod(i2s_dev_t *hw, uint32_t val)
{
    hw->fifo_conf.tx_fifo_mod = val;
}

static inline void i2s_out_tx_set_chan_mod(i2s_dev_t *hw, uint32_t val)
{
    hw->conf_chan.tx_chan_mod = val;
}

static inline void i2s_out_tx_enable_short_sync(i2s_dev_t *hw, bool en)
{
    hw->conf.tx_short_sync = en;
}
