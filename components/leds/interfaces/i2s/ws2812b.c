#include "../ws2812b.h"
#include "../../leds.h"
#include "../../stats.h"

#include <logging.h>

#define WS2812B_RESET_US 80

/*
 * Use 1.25us WS2812B bits divided into four periods in 1000/1110 form for 0/1 bits.
 *
 * https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
 */
 #define WS2812B_LUT(x) (\
     (((x >> 0) & 0x1) ? 0b0000000000001110 : 0b0000000000001000) \
   | (((x >> 1) & 0x1) ? 0b0000000011100000 : 0b0000000010000000) \
   | (((x >> 2) & 0x1) ? 0b0000111000000000 : 0b0000100000000000) \
   | (((x >> 3) & 0x1) ? 0b1110000000000000 : 0b1000000000000000) \
 )

static const uint16_t ws2812b_lut[] = {
  [0b0000] = WS2812B_LUT(0b0000),
  [0b0001] = WS2812B_LUT(0b0001),
  [0b0010] = WS2812B_LUT(0b0010),
  [0b0011] = WS2812B_LUT(0b0011),
  [0b0100] = WS2812B_LUT(0b0100),
  [0b0101] = WS2812B_LUT(0b0101),
  [0b0110] = WS2812B_LUT(0b0110),
  [0b0111] = WS2812B_LUT(0b0111),
  [0b1000] = WS2812B_LUT(0b1000),
  [0b1001] = WS2812B_LUT(0b1001),
  [0b1010] = WS2812B_LUT(0b1010),
  [0b1011] = WS2812B_LUT(0b1011),
  [0b1100] = WS2812B_LUT(0b1100),
  [0b1101] = WS2812B_LUT(0b1101),
  [0b1110] = WS2812B_LUT(0b1110),
  [0b1111] = WS2812B_LUT(0b1111),
};

int leds_tx_i2s_ws2812b_serial(const struct leds_interface_i2s_options *options, union ws2812b_pixel *pixels, unsigned count, struct leds_limit limit)
{
  struct leds_interface_i2s_stats *stats = &leds_interface_stats.i2s;
  struct i2s_out_options i2s_out_options = {
    .mode         = I2S_OUT_MODE_16BIT_SERIAL,

    // 3.2MHz bit clock => 0.3125us per I2S bit
    // four I2S bits per 1.25us WS2812B bit
    // two WS2812B bits per I2S byte
    .clock        = I2S_OUT_CLOCK_3M2,

    // four bytes per I2S sample, 10us per 32-bit I2S sample
    // hold low for 8 x 10us
    .eof_value    = 0x00000000,
    .eof_count    = 8,

    // shared IO pins
    .pin_mutex    = options->pin_mutex,
    .pin_timeout  = options->pin_timeout,
#if LEDS_I2S_GPIO_PINS_ENABLED
    .bck_gpio       = GPIO_NUM_NC,
    .data_gpio      = options->data_pin,
    .inv_data_gpio  = options->inv_data_pin,
#endif
  };
  int err;

  WITH_STATS_TIMER(&stats->open) {
    if ((err = i2s_out_open(options->i2s_out, i2s_out_options))) {
      LOG_ERROR("i2s_out_open");
      return err;
    }
  }

#if CONFIG_LEDS_GPIO_ENABLED
  if (options->gpio.gpio_out) {
    gpio_out_set(options->gpio.gpio_out, options->gpio.gpio_out_pins);
  }
#endif

  WITH_STATS_TIMER(&stats->tx) {
    for (unsigned i = 0; i < count; i++) {
      uint16_t buf[6];
      uint32_t grb = ws2812b_pixel_limit(pixels[i], limit)._grb;

      // 16-bit little-endian
      buf[0] = ws2812b_lut[(grb >> 20) & 0xf];
      buf[1] = ws2812b_lut[(grb >> 16) & 0xf];
      buf[2] = ws2812b_lut[(grb >> 12) & 0xf];
      buf[3] = ws2812b_lut[(grb >>  8) & 0xf];
      buf[4] = ws2812b_lut[(grb >>  4) & 0xf];
      buf[5] = ws2812b_lut[(grb >>  0) & 0xf];

      if ((err = i2s_out_write_serial16(options->i2s_out, buf, 6))) {
        LOG_ERROR("i2s_out_write_serial16");
        goto error;
      }
    }

    if ((err = i2s_out_flush(options->i2s_out))) {
      LOG_ERROR("i2s_out_flush");
      goto error;
    }
  }

error:
#if CONFIG_LEDS_GPIO_ENABLED
  if (options->gpio.gpio_out) {
    gpio_out_clear(options->gpio.gpio_out);
  }
#endif

  if ((err = i2s_out_close(options->i2s_out))) {
    LOG_ERROR("i2s_out_close");
    return err;
  }

  return err;
}

#if I2S_OUT_PARALLEL_SUPPORTED
  int leds_tx_i2s_ws2812b_parallel(const struct leds_interface_i2s_options *options, union ws2812b_pixel *pixels, unsigned count, struct leds_limit limit)
  {
    struct leds_interface_i2s_stats *stats = &leds_interface_stats.i2s;
    struct i2s_out_options i2s_out_options = {
      .mode         = I2S_OUT_MODE_8BIT_PARALLEL,

      // 3.2MHz bit clock => 0.3125us per I2S bit
      // four I2S bits per 1.25us WS2812B bit
      // two WS2812B bits per I2S byte
      .clock        = I2S_OUT_CLOCK_3M2,

      // four bytes per I2S sample, 10us per 32-bit I2S sample
      // hold low for 8 x 10us
      .eof_value    = 0x00000000,
      .eof_count    = 8,

      // shared IO pins
      .pin_mutex    = options->pin_mutex,
      .pin_timeout  = options->pin_timeout,
  #if LEDS_I2S_GPIO_PINS_ENABLED
      .bck_gpio     = GPIO_NUM_NC,
  #endif
    };
    int err;

    for (int i = 0; i < I2S_OUT_PARALLEL_SIZE; i++) {
      i2s_out_options.data_gpios[i] = (i < options->data_pins_count && i < LEDS_I2S_DATA_PINS_SIZE) ? options->data_pins[i] : GPIO_NUM_NC;
      i2s_out_options.inv_data_gpios[i] = (i < options->data_pins_count && i < LEDS_I2S_DATA_PINS_SIZE) ? options->inv_data_pins[i] : GPIO_NUM_NC;
    }

    WITH_STATS_TIMER(&stats->open) {
      if ((err = i2s_out_open(options->i2s_out, i2s_out_options))) {
        LOG_ERROR("i2s_out_open");
        return err;
      }
    }

  #if CONFIG_LEDS_GPIO_ENABLED
    if (options->gpio.gpio_out) {
      gpio_out_set(options->gpio.gpio_out, options->gpio.gpio_out_pins);
    }
  #endif

    WITH_STATS_TIMER(&stats->tx) {
      unsigned segment_size = options->data_pins_count; // number of pins
      unsigned segment_count = count / segment_size; // number of pixels per pin

      for (unsigned i = 0; i < segment_count; i++) {
        // 8 sets of 24x4-bit sub-pixels as 6x16-bit LUT values
        uint16_t buf[8][6] = {};

        for (unsigned j = 0; j < segment_size; j++) {
          uint32_t grb = ws2812b_pixel_limit(pixels[j * segment_count + i], limit)._grb;

          // 16-bit little-endian
          buf[j][0] = ws2812b_lut[(grb >> 20) & 0xf];
          buf[j][1] = ws2812b_lut[(grb >> 16) & 0xf];
          buf[j][2] = ws2812b_lut[(grb >> 12) & 0xf];
          buf[j][3] = ws2812b_lut[(grb >>  8) & 0xf];
          buf[j][4] = ws2812b_lut[(grb >>  4) & 0xf];
          buf[j][5] = ws2812b_lut[(grb >>  0) & 0xf];
        }

        // write each set of pixels
        if ((err = i2s_out_write_parallel8x16(options->i2s_out, buf, 6))) {
          LOG_ERROR("i2s_out_write_parallel8x16");
          goto error;
        }
      }

      if ((err = i2s_out_flush(options->i2s_out))) {
        LOG_ERROR("i2s_out_flush");
        goto error;
      }
    }

  error:
  #if CONFIG_LEDS_GPIO_ENABLED
    if (options->gpio.gpio_out) {
      gpio_out_clear(options->gpio.gpio_out);
    }
  #endif

    if ((err = i2s_out_close(options->i2s_out))) {
      LOG_ERROR("i2s_out_close");
      return err;
    }

    return err;
  }
#endif
