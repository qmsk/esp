#pragma once

#include <hal/i2s_ll.h>
#include <soc/i2s_reg.h>

static inline void i2s_intr_enable(i2s_dev_t *hw, uint32_t mask)
{
    hw->int_ena.val |= mask;
}

static inline void i2s_intr_clear(i2s_dev_t *hw, uint32_t mask)
{
    hw->int_clr.val = mask;
}

static inline void i2s_intr_disable(i2s_dev_t *hw, uint32_t mask)
{
    hw->int_ena.val &= ~mask;
}

static inline void i2s_intr_disable_all(i2s_dev_t *hw)
{
    hw->int_ena.val = 0;
}
