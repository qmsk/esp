#pragma once

#include <stddef.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

typedef int i2s_port_t;

#if CONFIG_IDF_TARGET_ESP8266
  #include <esp8266/eagle_soc.h>

  #define I2S_PORT_0      0
  #define I2S_PORT_MAX    1

  #define I2S_OUT_GPIO_PINS_SUPPORTED 0
  #define I2S_OUT_PARALLEL_SUPPORTED 0

  #define I2S_OUT_BASE_CLOCK (2 * APB_CLK_FREQ)

  enum i2s_out_mode {
    I2S_OUT_MODE_16BIT_SERIAL,  // 2x16-bit little-endian
    I2S_OUT_MODE_32BIT_SERIAL,  // 32-bit little-endian
  };

  // buffer_align required for various write modes
  #define I2S_OUT_WRITE_SERIAL16_ALIGN sizeof(uint16_t)
  #define I2S_OUT_WRITE_SERIAL32_ALIGN sizeof(uint32_t)

#elif CONFIG_IDF_TARGET_ESP32
  #include <hal/gpio_types.h>
  #include <soc/soc.h>

  #define I2S_PORT_0      0
  #define I2S_PORT_1      1
  #define I2S_PORT_MAX    2

  #define I2S_OUT_GPIO_PINS_SUPPORTED 1
  #define I2S_OUT_PARALLEL_SUPPORTED 1
  #define I2S_OUT_PARALLEL_SIZE 8

  #define I2S_OUT_BASE_CLOCK (2 * APB_CLK_FREQ)

  enum i2s_out_mode {
    I2S_OUT_MODE_16BIT_SERIAL,  // 2x16-bit little-endian
    I2S_OUT_MODE_32BIT_SERIAL,  // 32-bit little-endian
    I2S_OUT_MODE_8BIT_PARALLEL, // I2S1 (I2S_PORT_1) -only
  };

  // buffer_align required for various write modes
  #define I2S_OUT_WRITE_SERIAL16_ALIGN sizeof(uint16_t)
  #define I2S_OUT_WRITE_SERIAL32_ALIGN sizeof(uint32_t)
  #define I2S_OUT_WRITE_PARALLEL8X8_ALIGN sizeof(uint8_t[8])
  #define I2S_OUT_WRITE_PARALLEL8X16_ALIGN sizeof(uint16_t[8])
  #define I2S_OUT_WRITE_PARALLEL8X32_ALIGN sizeof(uint32_t[8])

#endif

struct i2s_out;

struct i2s_out_clock_options {
  // using the default non-APLL 160MHz clock
  uint32_t clkm_div   : 8; // master clock divider
  uint32_t bck_div    : 6; // bit clock divider
};

static const struct i2s_out_clock_options I2S_OUT_CLOCK_3M2 = { .clkm_div = 5, .bck_div = 10 };

/* Calculate approximate clkm/bck_div for given clock rate */
static inline struct i2s_out_clock_options i2s_out_clock(int rate)
{
  int div = I2S_OUT_BASE_CLOCK / rate;
  int mdiv = div % (1 << 6);

  // XXX: this results in invalid clock divisors for many inputs
  //      both clkm/bck_div must be >= 2
  //      bck_div must be < 64
  //      clkm_div must be < 256
  return (struct i2s_out_clock_options) {
    .clkm_div   = mdiv,
    .bck_div    = div / mdiv,
  };
}

struct i2s_out_options {
  enum i2s_out_mode mode;

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

#if I2S_OUT_GPIO_PINS_SUPPORTED
  // Use GPIO pin for bit clock out signal
  gpio_num_t bck_gpio; // -1 to disable
  bool bck_inv;

  // Use GPIO pin for data out signal, -1 to disable
  union {
    gpio_num_t data_gpio; // serial
# if I2S_OUT_PARALLEL_SUPPORTED
    gpio_num_t data_gpios[I2S_OUT_PARALLEL_SIZE]; // parallel
# endif
  };

  // Use GPIO pin for inverted data out signal, -1 to disable
  union {
    gpio_num_t inv_data_gpio; // serial
# if I2S_OUT_PARALLEL_SUPPORTED
    gpio_num_t inv_data_gpios[I2S_OUT_PARALLEL_SIZE]; // parallel
# endif
  };
#endif
};

/**
 * Allocate a new I2S output with an internal DMA TX buffer.
 *
 * The internal DMA buffers will hold `buffer_size` bytes, and be writable in chunks of `buffer_align` bytes.

 * The `buffer_align` MUST be a power of two.
 * The 32-bit hardware FIFO dictates a minimum 4-byte alignment, and `buffer_align` will be adjusted if necessary.
 */
int i2s_out_new(struct i2s_out **i2s_outp, i2s_port_t port, size_t buffer_size, size_t buffer_align);

/**
 * Setup the I2S output.
 *
 * Locks the i2s output for the calling task, use `i2s_out_close()` to release.
 *
 * Returns <0 on error, 0 on success.
 */
int i2s_out_open(struct i2s_out *i2s_out, struct i2s_out_options options);

/**
 * Copy exactly `count` 16-bit words from `data` into the internal TX DMA buffer for serial output in little-endian order.
 *
 * This does not yet start the I2S output. The internal TX DMA buffer only fits `buffer_size` bytes.
 * Use `i2s_out_flush()` / `i2s_out_close()` to start the I2S output and empty the TX DMA buffer.
 *
 * @param data 32-bit aligned data (LSB)
 * @param count number of 32-bit words
 *
 * Returns <0 error, 0 on success, >0 if TX buffer is full.
 */
int i2s_out_write_serial16(struct i2s_out *i2s_out, const uint16_t data[], size_t count);

/**
 * Copy exactly `count` 32-bit words from `data` into the internal TX DMA buffer for serial output in little-endian order.
 *
 * I2S TX uses 32-bit little-endian words, each set of 4 bytes is transmitted in least-significant-byte -> most-significant-bit first order.
 *
 * This does not yet start the I2S output. The internal TX DMA buffer only fits `buffer_size` bytes.
 * Use `i2s_out_flush()` / `i2s_out_close()` to start the I2S output and empty the TX DMA buffer.
 *
 * @param data 32-bit aligned data (LSB)
 * @param count number of 32-bit words
 *
 * Returns <0 error, 0 on success, >0 if TX buffer is full.
 */
int i2s_out_write_serial32(struct i2s_out *i2s_out, const uint32_t data[], size_t count);

#if I2S_OUT_PARALLEL_SUPPORTED
  /**
   * Copy exactly 8 channels of 8-bit `data` into the internal TX DMA buffer, transposing the buffers for
   * parallel output.
   *
   * This does not yet start the I2S output. The internal TX DMA buffer only fits `buffer_size` bytes.
   * Use `i2s_out_flush()` / `i2s_out_close()` to start the I2S output and empty the TX DMA buffer.
   *
   * @param data 8-bit data per channel
   *
   * Returns <0 error, 0 on success, >0 if TX buffer is full.
   */
  int i2s_out_write_parallel8x8(struct i2s_out *i2s_out, uint8_t data[8]);

  /**
   * Copy 8 channels of `width` x 16-bit `data`  into the internal TX DMA buffer, transposing the buffers for
   * parallel output.
   *
   * This does not yet start the I2S output. The internal TX DMA buffer only fits `buffer_size` bytes.
   * Use `i2s_out_flush()` / `i2s_out_close()` to start the I2S output and empty the TX DMA buffer.
   *
   * @param data[8][width] 16-bit data per channel
   * @param width number of uint16_t values per channel
   *
   * Returns <0 error, 0 on success, >0 if TX buffer is full.
   */
   int i2s_out_write_parallel8x16(struct i2s_out *i2s_out, uint16_t *data, unsigned width);

  /**
   * Copy 8 channels of `width` x 32-bit `data`  into the internal TX DMA buffer, transposing the buffers for
   * parallel output.
   *
   * This does not yet start the I2S output. The internal TX DMA buffer only fits `buffer_size` bytes.
   * Use `i2s_out_flush()` / `i2s_out_close()` to start the I2S output and empty the TX DMA buffer.
   *
   * @param data[8][width] 32-bit data per channel
   * @param width number of uint32_t values per channel
   *
   * Returns <0 error, 0 on success, >0 if TX buffer is full.
   */
   int i2s_out_write_parallel8x32(struct i2s_out *i2s_out, uint32_t *data, unsigned width);
#endif

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
