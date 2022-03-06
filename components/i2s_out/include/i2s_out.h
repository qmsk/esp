#pragma once

#include <stddef.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

typedef int i2s_port_t;

#if CONFIG_IDF_TARGET_ESP8266
# define I2S_PORT_0      0
# define I2S_PORT_MAX    1

# define I2S_OUT_OPTIONS_DATA_GPIO_ENABLED 0

#elif CONFIG_IDF_TARGET_ESP32
#include <hal/gpio_types.h>

# define I2S_PORT_0      0
# define I2S_PORT_1      1
# define I2S_PORT_MAX    2

# define I2S_OUT_OPTIONS_DATA_GPIO_ENABLED 1

#endif

struct i2s_out;

struct i2s_out_clock_options {
  // using the default 160MHz clock
  uint32_t clkm_div   : 6; // master clock divider
  uint32_t bck_div    : 6; // bit clock divider
};

static const struct i2s_out_clock_options I2S_DMA_CLOCK_3M2 = { .clkm_div = 5, .bck_div = 10 };

struct i2s_out_options {
  struct i2s_out_clock_options clock;

  // When the I2S TX FIFO empties, the I2S output will loop the last output value
  uint32_t eof_value;

  // The EOF value can be repeated to implement a reset frame
  unsigned eof_count;

  // Acquire mutex before configuring dev
  SemaphoreHandle_t dev_mutex;

  // Acquire mutex before setting pin funcs
  SemaphoreHandle_t pin_mutex;

  // Timeout for acquiring pin mutex, default 0 -> immediate error if pin in use
  TickType_t pin_timeout;

#if I2S_OUT_OPTIONS_DATA_GPIO_ENABLED
  // Use GPIO pin for data out signal
  gpio_num_t data_gpio;
#endif
};

/**
 * Allocate a new I2S output with an internal DMA TX buffer.
 */
int i2s_out_new(struct i2s_out **i2s_outp, i2s_port_t port, size_t buffer_size);

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
 *
 * Tears down the data_gpio pin.
 */
int i2s_out_close(struct i2s_out *i2s_out);

/**
 * Tear down all state setup by i2s_out_open(), including state typically shared across multiple open -> close calls.
 */
int i2s_out_teardown(struct i2s_out *i2s_out);
