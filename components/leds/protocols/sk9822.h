#pragma once

#include <leds.h>
#include "../protocol.h"
#include "../interface.h"
#include "../interfaces/i2s.h"
#include "../limit.h"

#define SK9822_GLOBAL_BYTE(brightness) (0xE0 | ((brightness) >> 3))
#define SK9822_BRIGHTNESS(global) (global & 0x1F) // 0..31

union __attribute__((packed)) sk9822_pixel {
  struct {
    uint8_t r, g, b;
    uint8_t global;
  };

  // aligned with 0xXXBBGGRR on little-endian architectures
  uint32_t xbgr;
};

// one frame of 0-bits
#define SK9822_START_FRAME_UINT32 (uint32_t) 0x00000000

// one bit per LED, in frames of 32 bits
#define SK9822_END_FRAME_UINT32 (uint32_t) 0x00000000
#define SK9822_END_FRAME_COUNT(count) (1 + count / 32)

#define LEDS_PROTOCOL_SK9822_INTERFACE_I2S_MODE LEDS_INTERFACE_I2S_MODE_32BIT_BCK

#define SK9822_PIXEL_POWER_DIVISOR (3 * 255 * 31) // one pixel at full brightness

static inline bool sk9822_pixel_active(const union sk9822_pixel pixel)
{
  return (pixel.b || pixel.g || pixel.r) && SK9822_BRIGHTNESS(pixel.global) > 0;
}

static inline unsigned sk9822_pixel_power(const union sk9822_pixel pixel)
{
  // use brightness to 0..31 to not overflow a 32-bit uint for a 16-bit LEDS_COUNT_MAX
  return (pixel.b + pixel.g + pixel.r) * SK9822_BRIGHTNESS(pixel.global);
}

static inline union sk9822_pixel sk9822_pixel_limit(const union sk9822_pixel pixel, struct leds_limit limit)
{
  return (union sk9822_pixel) {
    .global = pixel.global, // TODO: use driving current instead of PWM for power limit?
    .b      = leds_limit_uint8(limit, pixel.b),
    .g      = leds_limit_uint8(limit, pixel.g),
    .r      = leds_limit_uint8(limit, pixel.r),
  };
}

struct leds_protocol_sk9822 {
  union sk9822_pixel *pixels;
  unsigned count;
};

int leds_protocol_sk9822_init(struct leds_protocol_sk9822 *protocol, union leds_interface_state *interface, const struct leds_options *options);
int leds_protocol_sk9822_tx(struct leds_protocol_sk9822 *protocol, union leds_interface_state *interface, const struct leds_options *options, struct leds_limit limit);

void leds_protocol_sk9822_set(struct leds_protocol_sk9822 *protocol, unsigned index, struct leds_color color);
void leds_protocol_sk9822_set_all(struct leds_protocol_sk9822 *protocol, struct leds_color color);

unsigned leds_protocol_sk9822_count_active(struct leds_protocol_sk9822 *protocol);
unsigned leds_protocol_sk9822_count_power(struct leds_protocol_sk9822 *protocol, unsigned index, unsigned count);
