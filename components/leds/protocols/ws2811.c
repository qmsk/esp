#include "ws2811.h"
#include "../leds.h"
#include "../interfaces/i2s.h"
#include "../interfaces/uart.h"

#include <logging.h>

#include <stdlib.h>

union ws2811_pixel {
  struct {
    uint8_t b, g, r;
  };

  // aligned with 0xXXRRGGBB on little-endian architectures
  uint32_t _rgb;
};

static inline union ws2811_pixel ws2811_pixel(struct leds_color color, unsigned index, const struct leds_limit *limit)
{
  return (union ws2811_pixel) {
    .b  = leds_limit_uint8(limit, index, color.b),
    .g  = leds_limit_uint8(limit, index, color.g),
    .r  = leds_limit_uint8(limit, index, color.r),
  };
}

#if CONFIG_LEDS_I2S_ENABLED
  #define LEDS_PROTOCOL_WS2811_INTERFACE_I2S_MODE LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL

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

  static void leds_protocol_ws2811_i2s_out(uint16_t buf[6], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit)
  {
    union ws2811_pixel pixel = ws2811_pixel(pixels[index], index, limit);

    // 16-bit little-endian
    buf[0] = ws2811_lut[(pixel._rgb >> 20) & 0xf];
    buf[1] = ws2811_lut[(pixel._rgb >> 16) & 0xf];
    buf[2] = ws2811_lut[(pixel._rgb >> 12) & 0xf];
    buf[3] = ws2811_lut[(pixel._rgb >>  8) & 0xf];
    buf[4] = ws2811_lut[(pixel._rgb >>  4) & 0xf];
    buf[5] = ws2811_lut[(pixel._rgb >>  0) & 0xf];
  }
#endif

int leds_protocol_ws2811_init(union leds_interface_state *interface, const struct leds_options *options)
{
  int err;

  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      break;

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      if ((err = leds_interface_i2s_init(&interface->i2s, &options->i2s, LEDS_PROTOCOL_WS2811_INTERFACE_I2S_MODE, options->count))) {
        LOG_ERROR("leds_interface_i2s_init");
        return err;
      }

      break;
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return -1;
  }

  return 0;
}

int leds_protocol_ws2811_tx(union leds_interface_state *interface, const struct leds_options *options, const struct leds_color *pixels, const struct leds_limit *limit)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_UART_ENABLED
    case LEDS_INTERFACE_UART:
      return leds_tx_uart_ws2811(&options->uart, protocol->pixels, protocol->count, limit);
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      return leds_interface_i2s_tx(&interface->i2s, LEDS_INTERFACE_I2S_FUNC(i2s_mode_24bit_4x4, leds_protocol_ws2811_i2s_out), pixels, limit);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return -1;
  }
}

struct leds_protocol_type leds_protocol_ws2811 = {
#if CONFIG_LEDS_I2S_ENABLED
  .i2s_interface_mode  = LEDS_PROTOCOL_WS2811_INTERFACE_I2S_MODE,
#endif
  .parameter_type     = LEDS_PARAMETER_NONE,
  .power_mode         = LEDS_POWER_RGB,

  .init               = &leds_protocol_ws2811_init,
  .tx                 = &leds_protocol_ws2811_tx,
};
