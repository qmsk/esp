#include "../../sk6812grbw.h"
#include "../../leds.h"

#include <logging.h>

#define SK6812_RESET_US 80

/*
 * Use 1.25us SK6812 bits divided into four periods in 1000/1100 form for 0/1 bits.
 *
 * https://cdn-shop.adafruit.com/datasheets/SK6812.pdf
 */
 #define SK6812_LUT(x) (\
     (((x >> 0) & 0x1) ? 0b0000000000001100 : 0b0000000000001000) \
   | (((x >> 1) & 0x1) ? 0b0000000011000000 : 0b0000000010000000) \
   | (((x >> 2) & 0x1) ? 0b0000110000000000 : 0b0000100000000000) \
   | (((x >> 3) & 0x1) ? 0b1100000000000000 : 0b1000000000000000) \
 )

static const uint16_t sk6812_lut[] = {
  [0b0000] = SK6812_LUT(0b0000),
  [0b0001] = SK6812_LUT(0b0001),
  [0b0010] = SK6812_LUT(0b0010),
  [0b0011] = SK6812_LUT(0b0011),
  [0b0100] = SK6812_LUT(0b0100),
  [0b0101] = SK6812_LUT(0b0101),
  [0b0110] = SK6812_LUT(0b0110),
  [0b0111] = SK6812_LUT(0b0111),
  [0b1000] = SK6812_LUT(0b1000),
  [0b1001] = SK6812_LUT(0b1001),
  [0b1010] = SK6812_LUT(0b1010),
  [0b1011] = SK6812_LUT(0b1011),
  [0b1100] = SK6812_LUT(0b1100),
  [0b1101] = SK6812_LUT(0b1101),
  [0b1110] = SK6812_LUT(0b1110),
  [0b1111] = SK6812_LUT(0b1111),
};

int leds_tx_i2s_sk6812grbw(const struct leds_options *options, union sk6812grbw_pixel *pixels, unsigned count)
{
  struct i2s_out_options i2s_out_options = {
    // 3.2MHz bit clock => 0.3125us per I2S bit
    // four I2S bits per 1.25us SK6812 bit
    // two SK6812 bits per I2S byte
    .clock        = I2S_DMA_CLOCK_3M2,

    // four bytes per I2S sample, 10us per 32-bit I2S sample
    // hold low for 8 x 10us
    .eof_value    = 0x00000000,
    .eof_count    = 8,

    // shared IO pins
    .pin_mutex    = options->i2s_pin_mutex,
#if I2S_OUT_OPTIONS_DATA_GPIO
    .data_gpio    = options->i2s_gpio_pin,
#endif
  };
  uint16_t buf[8];
  int err;

  if ((err = i2s_out_open(options->i2s_out, i2s_out_options))) {
    LOG_ERROR("i2s_out_open");
    return err;
  }

#if CONFIG_LEDS_GPIO_ENABLED
  if (options->gpio_out) {
    gpio_out_set(options->gpio_out, options->gpio_out_pins);
  }
#endif

  for (unsigned i = 0; i < count; i++) {
    uint32_t grbw = pixels[i].grbw;

    buf[0] = sk6812_lut[(grbw >> 28) & 0xf];
    buf[1] = sk6812_lut[(grbw >> 24) & 0xf];
    buf[2] = sk6812_lut[(grbw >> 20) & 0xf];
    buf[3] = sk6812_lut[(grbw >> 16) & 0xf];
    buf[4] = sk6812_lut[(grbw >> 12) & 0xf];
    buf[5] = sk6812_lut[(grbw >>  8) & 0xf];
    buf[6] = sk6812_lut[(grbw >>  4) & 0xf];
    buf[7] = sk6812_lut[(grbw >>  0) & 0xf];

    if ((err = i2s_out_write_all(options->i2s_out, buf, sizeof(buf)))) {
      LOG_ERROR("i2s_out_write_all");
      goto error;
    }
  }

  if ((err = i2s_out_flush(options->i2s_out))) {
    LOG_ERROR("i2s_out_flush");
    return err;
  }

error:
#if CONFIG_LEDS_GPIO_ENABLED
  if (options->gpio_out) {
    gpio_out_clear(options->gpio_out);
  }
#endif

  if ((err = i2s_out_close(options->i2s_out))) {
    LOG_ERROR("i2s_out_close");
    return err;
  }

  return err;
}
