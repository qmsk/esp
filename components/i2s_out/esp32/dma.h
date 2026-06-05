#pragma once

// DMA buffer cannot be shorter than the I2S FIFO size (128 * 32-bit words)
#define DMA_DESC_SIZE_MIN (128 * 4)
#define DMA_DESC_SIZE_MAX ((1 << 12) - 4)

struct dma_desc {
  uint32_t size     : 12; // size of *buf in bytes
  uint32_t len      : 12; // number of bytes in *buf
  uint32_t _        : 5;  // unused
  uint32_t sub_sof  : 1;  // flag: 1 = sub-frame start
  uint32_t eof      : 1;  // flag: 1 = end of frame, triggers rx_eof interrupt

  volatile uint32_t owner    : 1;  // flag: 0 = software; 1 = hardware/DMA

  // word-aligned access data for I2S FIFO
  uint8_t *buf;

  // linked list
  struct dma_desc *next;
};

/* Prepare desc for re-use after DMA EOF */
static inline void i2s_dma_desc_reset(struct dma_desc *eof_desc)
{
  eof_desc->len = 0;
  eof_desc->owner = 0;
}

static inline void i2s_dma_tx_get_des_addr(i2s_dev_t *hw, uint32_t *dscr_addr)
{
    *dscr_addr = hw->out_link_dscr;
}
