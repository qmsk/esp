#include "i2s_out.h"
#include "slc.h"
#include "slc_isr.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>

#include <stdlib.h>
#include <string.h>

/* Allocate memory from appropriate heap region for DMA */
static inline void *slc_calloc(size_t count, size_t size)
{
  return heap_caps_calloc(count, size, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
}

void init_slc_desc(struct slc_desc *head, unsigned count, uint8_t *buf, size_t size)
{
  struct slc_desc **nextp = NULL;

  for (unsigned i = 0; i < count; i++) {
    struct slc_desc *desc = &head[i];

    if (nextp) {
      *nextp = desc;
    }

    desc->size = (size > (i + 1) * SLC_DESC_SIZE_MAX) ? SLC_DESC_SIZE_MAX : (size % SLC_DESC_SIZE_MAX);
    desc->len = 0;
    desc->owner = 0;
    desc->buf = buf ? &buf[i * SLC_DESC_SIZE_MAX] : NULL;

    LOG_DEBUG("i=%u desc=%p: size=%u buf=%p", i, desc, desc->size, desc->buf);

    nextp = &desc->next;
  }

  if (nextp) {
    // loop
    *nextp = head;
  }
}

int i2s_out_slc_init(struct i2s_out *i2s_out, size_t size)
{
  unsigned buf_size = size / sizeof(uint32_t);
  unsigned desc_count = size / SLC_DESC_SIZE_MAX;

  if (size % sizeof(uint32_t)) {
    buf_size += 1;
  }

  if (size % SLC_DESC_SIZE_MAX) {
    desc_count += 1;
  }

  LOG_DEBUG("size=%u -> buf_size=%u desc_count=%u", size, buf_size, desc_count);

  // allocate single word-aligned buffer
  if (!(i2s_out->slc_rx_buf = slc_calloc(buf_size, sizeof(uint32_t)))) {
    LOG_ERROR("slc_calloc(slc_rx_buf)");
    return -1;
  } else {
    LOG_DEBUG("slc_rx_buf=%p", i2s_out->slc_rx_buf);
  }

  // allocate DMA descriptors
  if (!(i2s_out->slc_rx_desc = slc_calloc(desc_count, sizeof(*i2s_out->slc_rx_desc)))) {
    LOG_ERROR("slc_calloc(slc_rx_desc)");
    return -1;
  }

  // initialize linked list of DMA descriptors
  init_slc_desc(i2s_out->slc_rx_desc, desc_count, i2s_out->slc_rx_buf, buf_size * sizeof(uint32_t));

  // setup isr
  slc_isr_mask();
  slc_isr_attach(i2s_out_slc_isr, i2s_out);

  return 0;
}

void IRAM_ATTR i2s_out_slc_isr(void *arg)
{
  struct i2s_out *i2s_out = arg;

  if (SLC0.int_st.rx_start) {
    LOG_ISR_DEBUG("rx_start");
  }
  if (SLC0.int_st.rx_udf) {
    LOG_ISR_DEBUG("rx_udf");
  }
  if (SLC0.int_st.rx_done) {
    LOG_ISR_DEBUG("rx_done");
  }
  if (SLC0.int_st.rx_eof) {
    LOG_ISR_DEBUG("rx_eof");

    slc_stop(&SLC0);
  }
  if (SLC0.int_st.rx_dscr_err) {
    LOG_ISR_DEBUG("rx_dscr_err");
  }

  slc_intr_clear(&SLC0);
}

void i2s_out_slc_setup(struct i2s_out *i2s_out, struct i2s_out_options options)
{
  LOG_DEBUG("...");

  taskENTER_CRITICAL();

  slc_stop(&SLC0);
  slc_intr_disable(&SLC0);
  slc_intr_clear(&SLC0);
  slc_reset(&SLC0);

  SLC0.conf0.txdscr_burst_en = 1;
  SLC0.conf0.txdata_burst_en = 0;

  SLC0.rx_dscr_conf.token_no_replace = 1;
  SLC0.rx_dscr_conf.infor_no_replace = 1;
  SLC0.rx_dscr_conf.rx_fill_mode = 0;
  SLC0.rx_dscr_conf.rx_eof_mode = 0;
  SLC0.rx_dscr_conf.rx_fill_en = 0;

  SLC0.rx_link.addr = (uint32_t) i2s_out->slc_rx_desc;
  SLC0.tx_link.addr = 0;

  taskEXIT_CRITICAL();

  // reset write pointer
  i2s_out->slc_write_desc = i2s_out->slc_rx_desc;
  i2s_out->slc_write_desc->len = 0;
  i2s_out->slc_write_desc->owner = 0;

  LOG_DEBUG("slc_write_desc=%p: owner=%u eof=%u len=%u size=%u", i2s_out->slc_write_desc, i2s_out->slc_write_desc->owner, i2s_out->slc_write_desc->eof, i2s_out->slc_write_desc->len, i2s_out->slc_write_desc->size);
}

int i2s_out_slc_write(struct i2s_out *i2s_out, void *buf, size_t size)
{
  struct slc_desc *desc = i2s_out->slc_write_desc;

  LOG_DEBUG("desc=%p (owner=%u eof=%u len=%u size=%u): size=%u", desc, desc->owner, desc->eof, desc->len, desc->size, size);

  if (desc->owner || desc->len >= desc->size) {
    // TODO: wait for DMA
    return 0;
  }

  if (size > desc->size || desc->len + size > desc->size) {
    size = desc->size - desc->len;
  }

  // copy data to desc buf
  LOG_DEBUG("copy size=%u -> buf=%p + len=%u", size, desc->buf, desc->len);

  memcpy(desc->buf + desc->len, buf, size);

  desc->len += size;

  // commit if full
  if (desc->len >= desc->size) {
    desc->owner = 1;

    LOG_DEBUG("commit desc=%p (owner=%u eof=%u len=%u size=%u) -> next=%p", desc, desc->owner, desc->eof, desc->len, desc->size, desc->next);

    i2s_out->slc_write_desc = desc->next;
  }

  return size;
}

void i2s_out_slc_start(struct i2s_out *i2s_out)
{
  i2s_out->slc_write_desc->eof = 1;
  i2s_out->slc_write_desc->owner = 1;

  LOG_DEBUG("slc_write_desc=%p: eof=%d owner=%d len=%u size=%u",
    i2s_out->slc_write_desc,
    i2s_out->slc_write_desc->eof,
    i2s_out->slc_write_desc->owner,
    i2s_out->slc_write_desc->len,
    i2s_out->slc_write_desc->size
  );

  taskENTER_CRITICAL();

  slc_intr_enable_rx(&SLC0);
  slc_start_rx(&SLC0);
  slc_isr_unmask();

  taskEXIT_CRITICAL();
}
