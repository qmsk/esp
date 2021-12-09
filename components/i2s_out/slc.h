#pragma once

#include <esp8266/slc_struct.h>
#include <esp8266/slc_register.h>

// DMA buffer cannot be shorter than the I2S FIFO size (128 * 32-bit words)
#define SLC_DESC_SIZE_MIN (128 * 4)
#define SLC_DESC_SIZE_MAX ((1 << 12) - 4)

struct slc_desc {
  uint32_t size     : 12; // size of *buf in bytes
  uint32_t len      : 12; // number of bytes in *buf
  uint32_t _        : 5;  // unused
  uint32_t sub_sof  : 1;  // flag: 1 = sub-frame start
  uint32_t eof      : 1;  // flag: 1 = end of frame, triggers rx_eof interrupt
  uint32_t owner    : 1;  // flag: 0 = software; 1 = hardware/DMA

  // word-aligned access data for I2S FIFO
  uint8_t *buf;

  // linked list
  struct slc_desc *next;
};

static inline void slc_reset(volatile slc_struct_t *slc)
{
  slc->conf0.val |= (SLC_TXLINK_RST | SLC_RXLINK_RST);
  slc->conf0.val &= ~(SLC_TXLINK_RST | SLC_RXLINK_RST);
}

static inline void slc_stop(volatile slc_struct_t *slc)
{
  slc->rx_link.stop = 1;
  slc->tx_link.stop = 1;
}

static inline void slc_start_rx(volatile slc_struct_t *slc)
{
  slc->rx_link.start = 1;
}

static inline void slc_intr_disable(volatile slc_struct_t *slc)
{
  slc->int_ena.rx_eof = 0;
  slc->int_ena.rx_dscr_err = 0;
}

static inline void slc_intr_enable_rx(volatile slc_struct_t *slc)
{
  slc->int_ena.rx_start = 0;
  slc->int_ena.rx_udf = 0;
  slc->int_ena.rx_done = 0; // TODO
  slc->int_ena.rx_eof = 1;
  slc->int_ena.rx_dscr_err = 1;
}

static inline void slc_intr_clear(volatile slc_struct_t *slc)
{
  slc->int_clr.val = slc->int_st.val;
}
