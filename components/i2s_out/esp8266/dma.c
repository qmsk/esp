#include "../i2s_out.h"
#include "slc.h"
#include "slc_isr.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>

#include <stdlib.h>
#include <string.h>

// align MUST be a power of two
// ref http://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
#define CHECK_ALIGN(align) ((align) && !((align) & ((align) - 1)))

// grow size to alignment
#define ALIGN(size, align) (((size) + ((align) - 1)) & ~((align) - 1))

// shrink size to aligment
#define TRUNC(size, align) ((size) & ~((align) - 1))

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

void init_dma_desc(struct dma_desc *head, unsigned count, uint8_t *buf, size_t size, size_t align, struct dma_desc *next)
{
  struct dma_desc **nextp = NULL;

  for (unsigned i = 0; i < count; i++) {
    struct dma_desc *desc = &head[i];

    if (nextp) {
      *nextp = desc;
    }

    desc->size = (size > TRUNC(DMA_DESC_SIZE_MAX, align)) ? TRUNC(DMA_DESC_SIZE_MAX, align) : size;
    desc->len = 0;
    desc->owner = 0;
    desc->buf = buf;

    LOG_DEBUG("i=%u desc=%p: size=%u buf=%p (align=%u size=%u)", i, desc, desc->size, desc->buf, align, size);

    buf += desc->size;
    size -= desc->size;

    nextp = &desc->next;
  }

  if (nextp) {
    // loop
    *nextp = next;
  }
}

/* Prepare desc for DMA start */
struct dma_desc *commit_dma_desc(struct dma_desc *desc)
{
  desc->owner = 1;

  return desc->next;
}

void init_dma_eof_desc(struct dma_desc *eof_desc, uint32_t value, unsigned count)
{
  uint32_t *ptr = (uint32_t *) eof_desc->buf;

  for (unsigned i = 0; i < count; i++) {
    ptr[i] = value;
  }

  eof_desc->len = count * sizeof(value);
  eof_desc->eof = 1;
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

int i2s_out_dma_init(struct i2s_out *i2s_out, size_t size, size_t align)
{
  size_t buf_size = 0;
  unsigned desc_count = 0;

  assert(CHECK_ALIGN(align)); // alignment MUST be a power of two

  // force 32-bit alignment for hardware FIFO
  align = ALIGN(align, sizeof(uint32_t));
  size = ALIGN(size, sizeof(uint32_t));

  // calculate buffer size for DMA descriptors
  for (desc_count = 0; buf_size < size; desc_count++) {
    if (buf_size + TRUNC(DMA_DESC_SIZE_MAX, align) <= size) {
      buf_size += TRUNC(DMA_DESC_SIZE_MAX, align);
    } else if (buf_size + ALIGN(DMA_DESC_SIZE_MIN, align) >= size) {
      buf_size += ALIGN(DMA_DESC_SIZE_MIN, align);
    } else {
      buf_size = size;
    }
  }

  LOG_DEBUG("size=%u align=%u -> desc_count=%u buf_size=%u", size, align, desc_count, buf_size);

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
  init_dma_desc(i2s_out->dma_rx_desc, desc_count, i2s_out->dma_rx_buf, buf_size, align, i2s_out->dma_eof_desc);
  init_dma_desc(i2s_out->dma_eof_desc, 1, i2s_out->dma_eof_buf, DMA_EOF_BUF_SIZE, sizeof(uint32_t), i2s_out->dma_eof_desc);

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

    // unblock flush() task
    xEventGroupSetBitsFromISR(i2s_out->event_group, I2S_OUT_EVENT_GROUP_BIT_DMA_EOF, &task_woken);
  }
  if (SLC0.int_st.rx_dscr_err) {
    LOG_ISR_WARN("rx_dscr_err");
  }

  if (task_woken == pdTRUE) {
    portYIELD_FROM_ISR();
  }

  slc_intr_clear(&SLC0);
}

int i2s_out_dma_setup(struct i2s_out *i2s_out, const struct i2s_out_options *options)
{
  LOG_DEBUG("...");

  if (options->eof_count * sizeof(options->eof_value) > DMA_EOF_BUF_SIZE) {
    LOG_ERROR("eof_count=%u is too large for eof buf size=%u", options->eof_count, DMA_EOF_BUF_SIZE);
    return -1;
  }

  // init EOF buffer
  init_dma_eof_desc(i2s_out->dma_eof_desc, options->eof_value, options->eof_count);

  // init RX desc
  reinit_dma_desc(i2s_out->dma_rx_desc, i2s_out->dma_rx_count, i2s_out->dma_eof_desc);

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

  SLC0.rx_link.addr = (uint32_t) i2s_out->dma_rx_desc;

  taskEXIT_CRITICAL();

  // reset eof state
  xEventGroupClearBits(i2s_out->event_group, I2S_OUT_EVENT_GROUP_BIT_DMA_EOF);

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

/*
 * Return a pointer into a DMA buffer capable of holding up to count * size bytes.
 *
 * @return size of usable buffer in units of size, up to count. 0 if full
 */
size_t i2s_out_dma_buffer(struct i2s_out *i2s_out, void **ptr, unsigned count, size_t size)
{
  for (;;) {
    struct dma_desc *desc = i2s_out->dma_write_desc;

    // hit dma_eof_desc?
    if (desc->owner || desc->eof) {
      LOG_DEBUG("eof desc=%p (owner=%u eof=%u buf=%p len=%u size=%u)", desc, desc->owner, desc->eof, desc->buf, desc->len, desc->size);

      // unable to find a usable DMA buffer, TX buffers full
      *ptr = NULL;

      // TODO: start DMA early and wait for a free buffer?
      return 0;
    }

    // can fit minimum size
    if (desc->len + size > desc->size) {
      LOG_DEBUG("commit desc=%p (owner=%u eof=%u buf=%p len=%u size=%u) < size=%u -> next=%p", desc, desc->owner, desc->eof, desc->buf, desc->len, desc->size, size, desc->next);

      // commit, try with the next buffer
      i2s_out->dma_write_desc = commit_dma_desc(desc);

      continue;
    }

    if (desc->len + count * size > desc->size) {
      LOG_DEBUG("limited desc=%p (owner=%u eof=%u buf=%p len=%u size=%u) < count=%u size=%u", desc, desc->owner, desc->eof, desc->buf, desc->len, desc->size, count, size);

      // limit to available buffer size
      count = (desc->size - desc->len) / size;

    } else {
      LOG_DEBUG("complete desc=%p (owner=%u eof=%u buf=%p len=%u size=%u) >= count=%u size=%u", desc, desc->owner, desc->eof, desc->buf, desc->len, desc->size, count, size);
    }

    *ptr = desc->buf + desc->len;

    LOG_DEBUG("return ptr=%p count=%u size=%u", *ptr, count, size);

    return count;
  }
}

void i2s_out_dma_commit(struct i2s_out *i2s_out, unsigned count, size_t size)
{
  struct dma_desc *desc = i2s_out->dma_write_desc;

  desc->len += count * size;
}

int i2s_out_dma_write(struct i2s_out *i2s_out, const void *data, size_t size)
{
  void *ptr;
  int len = i2s_out_dma_buffer(i2s_out, &ptr, size, 1); // single unaligned bytes

  if (len) {
    // copy data to desc buf
    LOG_DEBUG("copy len=%u -> ptr=%p", len, ptr);

    memcpy(ptr, data, len);

    i2s_out_dma_commit(i2s_out, len, 1);
  }

  return len;
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
  i2s_out->dma_eof_desc->next = i2s_out->dma_eof_desc;

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

  slc_intr_clear(&SLC0);
  slc_intr_enable_rx(&SLC0);
  slc_isr_unmask();

  slc_start_rx(&SLC0);

  taskEXIT_CRITICAL();

  i2s_out->dma_start = true;
}

int i2s_out_dma_flush(struct i2s_out *i2s_out)
{
  LOG_DEBUG("wait event_group bits=%08x", I2S_OUT_EVENT_GROUP_BIT_DMA_EOF);

  xEventGroupWaitBits(i2s_out->event_group, I2S_OUT_EVENT_GROUP_BIT_DMA_EOF, false, false, portMAX_DELAY);

  LOG_DEBUG("wait done");

  return 0;
}
