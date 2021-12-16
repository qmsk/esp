#pragma once

#include <stddef.h>
#include <stdint.h>

struct i2s_out;

struct i2s_out_clock_options {
  uint32_t clkm_div   : 6; // clock divider
  uint32_t bck_div    : 6; // clock divider
};

static const struct i2s_out_clock_options I2S_DMA_CLOCK_3M2 = { .clkm_div = 5, .bck_div = 10 };

struct i2s_out_options {
  struct i2s_out_clock_options clock;

  // When the I2S TX FIFO empties, the I2S output will loop the last output value
  uint32_t eof_value;

  // The EOF value can be repeated to implement a reset frame
  unsigned eof_count;
};

/**
 * Allocate a new I2S output with an internal DMA TX buffer.
 */
int i2s_out_new(struct i2s_out **i2s_outp, size_t buffer_size);

/**
 * Setup the I2S output.
 *
 * Locks the i2s output for the calling task, use `i2s_out_close()` to release.
 *
 * Returns <0 on error, 0 on success.
 */
int i2s_out_open(struct i2s_out *i2s_out, struct i2s_out_options options);

/**
 * Copy up to `len` bytes from `data` into the internal TX DMA buffer.
 *
 * This does not yet start the I2S output. The internal TX DMA buffer only fits `buffer_size` bytes.
 * Use `i2s_out_flush()` / `i2s_out_close()` to start the I2S output and empty the TX DMA buffer.
 *
 * Returns <0 on error, 0 if TX buffer full, >0 number of bytes written.
 */
int i2s_out_write(struct i2s_out *i2s_out, void *data, size_t len);

/**
* Copy exactly `len` bytes from `data` into the internal TX DMA buffer.
 *
 * This does not yet start the I2S output. The internal TX DMA buffer only fits `buffer_size` bytes.
 * Use `i2s_out_flush()` / `i2s_out_close()` to start the I2S output and empty the TX DMA buffer.
 *
 * Returns <0 error, 0 on success, >0 if TX buffer is full.
 */
int i2s_out_write_all(struct i2s_out *i2s_out, void *data, size_t len);

/**
 * Start I2S output, and wait for the complete TX buffer and EOF frame to be written.
 *
 * Returns <0 on error, 0 on success.
 */
int i2s_out_flush(struct i2s_out *i2s_out);

/**
 * Flush I2S output, and release the lock acquired using `i2s_out_open()`.
 */
int i2s_out_close(struct i2s_out *i2s_out);