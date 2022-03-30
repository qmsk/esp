#include "../ws2811.h"
#include "../../leds.h"
#include "../../stats.h"

#include <logging.h>

#define WS2811_RESET_US 80

/*
 * Use 1.25us WS2811 bits divided into four periods in 1000/1100 form for 0/1 bits.
 *
 * https://cdn-shop.adafruit.com/datasheets/WS2811.pdf
 */
 #define WS2811_LUT(x) (\
     (((x >> 0) & 0x1) ? 0b0000000000001100 : 0b0000000000001000) \
   | (((x >> 1) & 0x1) ? 0b0000000011000000 : 0b0000000010000000) \
   | (((x >> 2) & 0x1) ? 0b0000110000000000 : 0b0000100000000000) \
   | (((x >> 3) & 0x1) ? 0b1100000000000000 : 0b1000000000000000) \
 )

static const uint16_t ws2811_lut[] = {
  [0b0000] = WS2811_LUT(0b0000),
  [0b0001] = WS2811_LUT(0b0001),
  [0b0010] = WS2811_LUT(0b0010),
  [0b0011] = WS2811_LUT(0b0011),
  [0b0100] = WS2811_LUT(0b0100),
  [0b0101] = WS2811_LUT(0b0101),
  [0b0110] = WS2811_LUT(0b0110),
  [0b0111] = WS2811_LUT(0b0111),
  [0b1000] = WS2811_LUT(0b1000),
  [0b1001] = WS2811_LUT(0b1001),
  [0b1010] = WS2811_LUT(0b1010),
  [0b1011] = WS2811_LUT(0b1011),
  [0b1100] = WS2811_LUT(0b1100),
  [0b1101] = WS2811_LUT(0b1101),
  [0b1110] = WS2811_LUT(0b1110),
  [0b1111] = WS2811_LUT(0b1111),
};

int leds_tx_i2s_ws2811(const struct leds_interface_i2s_options *options, union ws2811_pixel *pixels, unsigned count, struct leds_limit limit)
{
  struct leds_interface_i2s_stats *stats = &leds_interface_stats.i2s;
  struct i2s_out_options i2s_out_options = {
    // 3.2MHz bit clock => 0.3125us per I2S bit
    // four I2S bits per 1.25us WS2811 bit
    // two WS2811 bits per I2S byte
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
    .data_gpio    = options->data_pin,
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
      uint32_t buf[3];
      uint32_t rgb = ws2811_pixel_limit(pixels[i], limit)._rgb;

      // 32-bit little-endian
      buf[0] = (ws2811_lut[(rgb >> 20) & 0xf] << 16) | (ws2811_lut[(rgb >> 16) & 0xf]);
      buf[1] = (ws2811_lut[(rgb >> 12) & 0xf] << 16) | (ws2811_lut[(rgb >>  8) & 0xf]);
      buf[2] = (ws2811_lut[(rgb >>  4) & 0xf] << 16) | (ws2811_lut[(rgb >>  0) & 0xf]);

      if ((err = i2s_out_write_all(options->i2s_out, buf, sizeof(buf)))) {
        LOG_ERROR("i2s_out_write_all");
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
