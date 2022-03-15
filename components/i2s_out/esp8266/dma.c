#include "../i2s_out.h"
#include "slc.h"
#include "slc_isr.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>

#include <stdlib.h>
#include <string.h>

#define ALIGN(size, type) (((size) + (sizeof(type) - 1)) & ~(sizeof(type) - 1))

#define DMA_EOF_BUF_SIZE (DMA_DESC_SIZE_MIN)

/* Allocate memory from appropriate heap region for DMA */
static inline void *dma_malloc(size_t size)
{
  return heap_caps_malloc(size, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
}

/* Allocate memory from appropriate heap region for DMA */
static inline void *dma_calloc(size_t count, size_t size)
{
  return heap_caps_calloc(count, size, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
}

void init_dma_desc(struct dma_desc *head, unsigned count, uint8_t *buf, size_t size, struct dma_desc *next)
{
  struct dma_desc **nextp = NULL;

  for (unsigned i = 0; i < count; i++) {
    struct dma_desc *desc = &head[i];

    if (nextp) {
      *nextp = desc;
    }

    desc->size = (size > DMA_DESC_SIZE_MAX) ? DMA_DESC_SIZE_MAX : size;
    desc->len = 0;
    desc->owner = 0;
    desc->buf = buf;

    LOG_DEBUG("i=%u desc=%p: size=%u buf=%p", i, desc, desc->size, desc->buf);

    buf += desc->size;
    size -= desc->size;

    nextp = &desc->next;
  }

  if (nextp) {
    // loop
    *nextp = next;
  }
}

void init_dma_eof_desc(struct dma_desc *eof_desc, uint32_t value, unsigned count)
{
  uint32_t *ptr = (uint32_t *) eof_desc->buf;

  for (unsigned i = 0; i < count; i++) {
    ptr[i] = value;
  }

  eof_desc->len = count * sizeof(value);
}

void reinit_dma_desc(struct dma_desc *head, unsigned count, struct dma_desc *next)
{
  struct dma_desc **nextp = NULL;

  for (unsigned i = 0; i < count; i++) {
    struct dma_desc *desc = &head[i];

    if (nextp) {
      *nextp = desc;
    }

    desc->len = 0;
    desc->owner = 0;

    nextp = &desc->next;
  }

  if (nextp) {
    // loop
    *nextp = next;
  }
}

int i2s_out_dma_init(struct i2s_out *i2s_out, size_t size)
{
  size_t buf_size = 0;
  unsigned desc_count = 0;

  // calculate buffer size for DMA descriptors
  for (desc_count = 0; buf_size < size; desc_count++) {
    if (buf_size + DMA_DESC_SIZE_MAX <= size) {
      buf_size += DMA_DESC_SIZE_MAX;
    } else if (buf_size + DMA_DESC_SIZE_MIN >= size) {
      buf_size += DMA_DESC_SIZE_MIN;
    } else {
      buf_size = size;
    }
  }

  // force 32-bit aligned
  buf_size = ALIGN(buf_size, uint32_t);

  LOG_DEBUG("size=%u -> desc_count=%u buf_size=%u", size, desc_count, buf_size);

  // allocate single word-aligned buffer
  if (!(i2s_out->dma_rx_buf = dma_malloc(buf_size))) {
    LOG_ERROR("dma_malloc(dma_rx_buf)");
    return -1;
  } else {
    LOG_DEBUG("dma_rx_buf=%p[%u]", i2s_out->dma_rx_buf, buf_size);
  }
  if (!(i2s_out->dma_eof_buf = dma_malloc(DMA_EOF_BUF_SIZE))) {
    LOG_ERROR("dma_malloc(dma_eof_buf)");
    return -1;
  } else {
    LOG_DEBUG("dma_eof_buf=%p[%u]", i2s_out->dma_eof_buf, DMA_EOF_BUF_SIZE);
  }

  // allocate DMA descriptors
  if (!(i2s_out->dma_rx_desc = dma_calloc(desc_count, sizeof(*i2s_out->dma_rx_desc)))) {
    LOG_ERROR("dma_calloc(dma_rx_desc)");
    return -1;
  }
  if (!(i2s_out->dma_eof_desc = dma_calloc(1, sizeof(*i2s_out->dma_eof_desc)))) {
    LOG_ERROR("dma_calloc(dma_eof_desc)");
    return -1;
  }

  // initialize linked list of DMA descriptors
  init_dma_desc(i2s_out->dma_rx_desc, desc_count, i2s_out->dma_rx_buf, buf_size, i2s_out->dma_eof_desc);
  init_dma_desc(i2s_out->dma_eof_desc, 1, i2s_out->dma_eof_buf, DMA_EOF_BUF_SIZE, i2s_out->dma_eof_desc);

  i2s_out->dma_rx_count = desc_count;

  // setup isr
  slc_isr_mask();
  slc_isr_attach(i2s_out_slc_isr, i2s_out);

  return 0;
}

void IRAM_ATTR i2s_out_slc_isr(void *arg)
{
  struct i2s_out *i2s_out = arg;
  BaseType_t task_woken = pdFALSE;

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
    struct dma_desc *eof_desc = slc_rx_eof_desc(&SLC0);

    LOG_ISR_DEBUG("rx_eof desc=%p", eof_desc);

    // NOTE: this is unlikely to stop DMA before this repeats at least once
    slc_stop(&SLC0);

    // mark as done
    i2s_out->dma_eof = true;

    // notify flush() if waiting
    if (i2s_out->dma_flush_task) {
      LOG_ISR_DEBUG("rx_eof notify task=%p", i2s_out->dma_flush_task);

      vTaskNotifyGiveFromISR(i2s_out->dma_flush_task, &task_woken);

      i2s_out->dma_flush_task = NULL;
    }
  }
  if (SLC0.int_st.rx_dscr_err) {
    LOG_ISR_DEBUG("rx_dscr_err");
  }

  if (task_woken == pdTRUE) {
    portYIELD_FROM_ISR();
  }

  slc_intr_clear(&SLC0);
}

int i2s_out_dma_setup(struct i2s_out *i2s_out, struct i2s_out_options options)
{
  LOG_DEBUG("...");

  if (options.eof_count * sizeof(options.eof_value) > DMA_EOF_BUF_SIZE) {
    LOG_ERROR("eof_count=%u is too large for eof buf size=%u", options.eof_count, DMA_EOF_BUF_SIZE);
    return -1;
  }

  // init EOF buffer
  init_dma_eof_desc(i2s_out->dma_eof_desc, options.eof_value, options.eof_count);

  // init RX desc
  reinit_dma_desc(i2s_out->dma_rx_desc, i2s_out->dma_rx_count, i2s_out->dma_eof_desc);

  taskENTER_CRITICAL();

  slc_stop(&SLC0);
  slc_intr_disable(&SLC0);
  slc_intr_clear(&SLC0);
  slc_reset(&SLC0);

  i2s_out->dma_eof = false; // flag set by ISR

  SLC0.conf0.txdscr_burst_en = 1;
  SLC0.conf0.txdata_burst_en = 0;

  SLC0.rx_dscr_conf.token_no_replace = 1;
  SLC0.rx_dscr_conf.infor_no_replace = 1;
  SLC0.rx_dscr_conf.rx_fill_mode = 0;
  SLC0.rx_dscr_conf.rx_eof_mode = 0;
  SLC0.rx_dscr_conf.rx_fill_en = 0;

  SLC0.rx_link.addr = (uint32_t) i2s_out->dma_rx_desc;

  taskEXIT_CRITICAL();

  // reset write desc
  i2s_out->dma_start = false;
  i2s_out->dma_write_desc = i2s_out->dma_rx_desc;

  LOG_DEBUG("dma_write_desc=%p: owner=%d eof=%d len=%u size=%u -> buf=%p next=%p",
    i2s_out->dma_write_desc,
    i2s_out->dma_write_desc->owner,
    i2s_out->dma_write_desc->eof,
    i2s_out->dma_write_desc->len,
    i2s_out->dma_write_desc->size,
    i2s_out->dma_write_desc->buf,
    i2s_out->dma_write_desc->next
  );

  LOG_DEBUG("dma_eof_desc=%p: owner=%d eof=%d len=%u size=%u -> buf=%p next=%p",
    i2s_out->dma_eof_desc,
    i2s_out->dma_eof_desc->owner,
    i2s_out->dma_eof_desc->eof,
    i2s_out->dma_eof_desc->len,
    i2s_out->dma_eof_desc->size,
    i2s_out->dma_eof_desc->buf,
    i2s_out->dma_eof_desc->next
  );

  return 0;
}

int i2s_out_dma_write(struct i2s_out *i2s_out, void *buf, size_t size)
{
  struct dma_desc *desc = i2s_out->dma_write_desc;

  LOG_DEBUG("desc=%p (owner=%u eof=%u len=%u size=%u): size=%u", desc, desc->owner, desc->eof, desc->len, desc->size, size);

  if (desc->owner || desc->eof || desc->len >= desc->size) {
    // XXX: TX buffers full, wait for DMA?
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

    i2s_out->dma_write_desc = desc->next;
  }

  return size;
}

int i2s_out_dma_pending(struct i2s_out *i2s_out)
{
  if (i2s_out->dma_start) {
    // start() already haṕpened
    return 0;
  }

  if (i2s_out->dma_write_desc != i2s_out->dma_rx_desc || i2s_out->dma_write_desc->len > 0) {
    // write() happened
    return 1;
  }

  return 0;
}

void i2s_out_dma_start(struct i2s_out *i2s_out)
{
  i2s_out->dma_write_desc->owner = 1;
  i2s_out->dma_write_desc->next = i2s_out->dma_eof_desc;

  i2s_out->dma_eof_desc->owner = 1;
  i2s_out->dma_eof_desc->eof = 1;

  LOG_DEBUG("dma_write_desc=%p: owner=%d eof=%d len=%u size=%u -> buf=%p next=%p",
    i2s_out->dma_write_desc,
    i2s_out->dma_write_desc->owner,
    i2s_out->dma_write_desc->eof,
    i2s_out->dma_write_desc->len,
    i2s_out->dma_write_desc->size,
    i2s_out->dma_write_desc->buf,
    i2s_out->dma_write_desc->next
  );

  LOG_DEBUG("dma_eof_desc=%p: owner=%d eof=%d len=%u size=%u -> buf=%p next=%p",
    i2s_out->dma_eof_desc,
    i2s_out->dma_eof_desc->owner,
    i2s_out->dma_eof_desc->eof,
    i2s_out->dma_eof_desc->len,
    i2s_out->dma_eof_desc->size,
    i2s_out->dma_eof_desc->buf,
    i2s_out->dma_eof_desc->next
  );

  taskENTER_CRITICAL();

  slc_intr_enable_rx(&SLC0);
  slc_start_rx(&SLC0);
  slc_isr_unmask();

  taskEXIT_CRITICAL();

  i2s_out->dma_start = true;
}

int i2s_out_dma_flush(struct i2s_out *i2s_out)
{
  int wait = 0;

  taskENTER_CRITICAL();

  if (!i2s_out->dma_eof) {
    wait = 1;

    i2s_out->dma_flush_task = xTaskGetCurrentTaskHandle();
  }

  taskEXIT_CRITICAL();

  if (!wait) {
    LOG_DEBUG("done eof");

    return 0;
  }

  LOG_DEBUG("wait eof, task=%p", i2s_out->dma_flush_task);

  // wait for tx to complete and break to start
  if (!ulTaskNotifyTake(true, portMAX_DELAY)) {
    LOG_WARN("ulTaskNotifyTake: timeout");
    return -1;
  }

  return 0;
}
