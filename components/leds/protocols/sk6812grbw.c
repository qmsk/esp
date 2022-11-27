#include "sk6812grbw.h"
#include "../leds.h"
#include "../interfaces/i2s.h"
#include "../interfaces/uart.h"

#include <logging.h>

#include <stdlib.h>

union sk6812grbw_pixel {
  struct {
    uint8_t w, b, r, g;
  };

  // aligned with 0xGGRRBBWW on little-endian architectures
  uint32_t grbw;
};

static inline union sk6812grbw_pixel sk6812grbw_pixel(struct leds_color color, unsigned index, const struct leds_limit *limit)
{
  return (union sk6812grbw_pixel) {
    .w  = leds_limit_uint8(limit, index, color.white),
    .b  = leds_limit_uint8(limit, index, color.b),
    .r  = leds_limit_uint8(limit, index, color.r),
    .g  = leds_limit_uint8(limit, index, color.g),
  };
}

#if CONFIG_LEDS_I2S_ENABLED
  #define LEDS_PROTOCOL_SK6812_GRBW_INTERFACE_I2S_MODE LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL

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

  static void leds_protocol_sk6812grbw_i2s_out(uint16_t buf[8], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit)
  {
    union sk6812grbw_pixel pixel = sk6812grbw_pixel(pixels[index], index, limit);

    // 16-bit little-endian
    buf[0] = sk6812_lut[(pixel.grbw >> 28) & 0xf];
    buf[1] = sk6812_lut[(pixel.grbw >> 24) & 0xf];
    buf[2] = sk6812_lut[(pixel.grbw >> 20) & 0xf];
    buf[3] = sk6812_lut[(pixel.grbw >> 16) & 0xf];
    buf[4] = sk6812_lut[(pixel.grbw >> 12) & 0xf];
    buf[5] = sk6812_lut[(pixel.grbw >>  8) & 0xf];
    buf[6] = sk6812_lut[(pixel.grbw >>  4) & 0xf];
    buf[7] = sk6812_lut[(pixel.grbw >>  0) & 0xf];
  }
#endif

int leds_protocol_sk6812grbw_init(union leds_interface_state *interface, const struct leds_options *options)
{
  int err;

  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      break;

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      if ((err = leds_interface_i2s_init(&interface->i2s, &options->i2s, LEDS_PROTOCOL_SK6812_GRBW_INTERFACE_I2S_MODE, options->count))) {
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

int leds_protocol_sk6812grbw_tx(union leds_interface_state *interface, const struct leds_options *options, const struct leds_color *pixels, const struct leds_limit *limit)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_UART_ENABLED
    case LEDS_INTERFACE_UART:
      return leds_tx_uart_sk6812grbw(&options->uart, protocol->pixels, protocol->count, limit);
  #endif

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      return leds_interface_i2s_tx(&interface->i2s, LEDS_INTERFACE_I2S_FUNC(i2s_mode_32bit_4x4, leds_protocol_sk6812grbw_i2s_out), pixels, limit);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return -1;
  }
}

struct leds_protocol_type leds_protocol_sk6812grbw = {
#if CONFIG_LEDS_I2S_ENABLED
  .i2s_interface_mode  = LEDS_PROTOCOL_SK6812_GRBW_INTERFACE_I2S_MODE,
#endif
  .parameter_type     = LEDS_PARAMETER_WHITE,
  .power_mode         = LEDS_POWER_RGB2W,

  .init               = &leds_protocol_sk6812grbw_init,
  .tx                 = &leds_protocol_sk6812grbw_tx,
};
