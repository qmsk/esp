#include "ws2812b.h"
#include "spi_leds.h"

#include <logging.h>

#define WS2812B_RESET_US 80

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

static const struct i2s_out_options i2s_out_options = {
  // 1.25us per WS2812B bit => 10us per byte
  .clock        = I2S_DMA_CLOCK_3M2,

  // hold low for 2 x 40us
  .eof_value    = 0x00000000,
  .eof_count    = 2,
};

int spi_leds_tx_i2s_ws2812b(const struct spi_leds_options *options, union ws2812b_pixel *pixels, unsigned count)
{
  uint16_t buf[6];
  int err;

  if ((err = i2s_out_open(options->i2s_out, i2s_out_options))) {
    LOG_ERROR("i2s_out_open");
    return err;
  }

  if (options->gpio_out) {
    gpio_out_set(options->gpio_out, options->gpio_out_pins);
  }

  for (unsigned i = 0; i < count; i++) {
    uint32_t grb = pixels[i]._grb;

    buf[0] = ws2812b_lut[(grb >> 20) & 0xf];
    buf[1] = ws2812b_lut[(grb >> 16) & 0xf];
    buf[2] = ws2812b_lut[(grb >> 12) & 0xf];
    buf[3] = ws2812b_lut[(grb >>  8) & 0xf];
    buf[4] = ws2812b_lut[(grb >>  4) & 0xf];
    buf[5] = ws2812b_lut[(grb >>  0) & 0xf];

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
  if (options->gpio_out) {
    gpio_out_clear(options->gpio_out);
  }

  if ((err = i2s_out_close(options->i2s_out))) {
    LOG_ERROR("i2s_out_close");
    return err;
  }

  return err;
}
