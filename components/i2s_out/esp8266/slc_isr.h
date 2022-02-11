#pragma once

#include "freertos/FreeRTOS.h"
#include "rom/ets_sys.h"

static inline void slc_isr_unmask()
{
  _xt_isr_unmask(1 << ETS_SLC_INUM);
}

static inline void slc_isr_mask()
{
  _xt_isr_mask(1 << ETS_SLC_INUM);
}

static inline void slc_isr_attach(_xt_isr func, void *arg)
{
  _xt_isr_attach(ETS_SLC_INUM, func, arg);
}

void IRAM_ATTR i2s_out_slc_isr(void *arg);
