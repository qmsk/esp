#include "ws2811.h"
#include "../leds.h"
#include "../interfaces/i2s.h"
#include "../interfaces/uart.h"

#include <logging.h>

#include <stdlib.h>

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

static void leds_protocol_ws2811_i2s_out(uint16_t buf[6], void *data, unsigned index, struct leds_limit limit)
{
  union ws2811_pixel *pixels = data;
  uint32_t rgb = ws2811_pixel_limit(pixels[index], limit)._rgb;

  // 16-bit little-endian
  buf[0] = ws2811_lut[(rgb >> 20) & 0xf];
  buf[1] = ws2811_lut[(rgb >> 16) & 0xf];
  buf[2] = ws2811_lut[(rgb >> 12) & 0xf];
  buf[3] = ws2811_lut[(rgb >>  8) & 0xf];
  buf[4] = ws2811_lut[(rgb >>  4) & 0xf];
  buf[5] = ws2811_lut[(rgb >>  0) & 0xf];
}

int leds_protocol_ws2811_init(struct leds_protocol_ws2811 *protocol, union leds_interface_state *interface, const struct leds_options *options)
{
  if (!(protocol->pixels = calloc(options->count, sizeof(*protocol->pixels)))) {
    LOG_ERROR("malloc");
    return -1;
  }

  protocol->count = options->count;

  return 0;
}

int leds_protocol_ws2811_tx(struct leds_protocol_ws2811 *protocol, union leds_interface_state *interface, const struct leds_options *options, struct leds_limit limit)
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
      return leds_interface_i2s_tx(&options->i2s, LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL, (struct leds_interface_i2s_tx) {
        .data   = protocol->pixels,
        .count  = protocol->count,
        .limit  = limit,

        .func.i2s_mode_24bit_4x4 = leds_protocol_ws2811_i2s_out,
      });
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return -1;
  }
}

void leds_protocol_ws2811_set(struct leds_protocol_ws2811 *protocol, unsigned index, struct leds_color color)
{
  if (index < protocol->count) {
    protocol->pixels[index] = (union ws2811_pixel) {
      .b = color.b,
      .g = color.g,
      .r = color.r,
    };
  }
}

void leds_protocol_ws2811_set_all(struct leds_protocol_ws2811 *protocol, struct leds_color color)
{
  for (unsigned index = 0; index < protocol->count; index++) {
    protocol->pixels[index] = (union ws2811_pixel) {
      .b = color.b,
      .g = color.g,
      .r = color.r,
    };
  }
}

unsigned leds_protocol_ws2811_count_active(struct leds_protocol_ws2811 *protocol)
{
  unsigned active = 0;

  for (unsigned index = 0; index < protocol->count; index++) {
    if (ws2811_pixel_active(protocol->pixels[index])) {
      active++;
    }
  }

  return active;
}

unsigned leds_protocol_ws2811_count_total(struct leds_protocol_ws2811 *protocol)
{
  unsigned total = 0;

  for (unsigned index = 0; index < protocol->count; index++) {
    total += ws2811_pixel_total(protocol->pixels[index]);
  }

  return total / WS2811_PIXEL_TOTAL_DIVISOR;
}
