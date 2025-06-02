#pragma once

#include <leds.h>
#include "limit.h"

union leds_pixel_rgb {
    struct {
      uint8_t b, g, r;
    };
  
    // aligned with 0xXXRRGGBB on little-endian architectures
    uint32_t _rgb;
};

static inline union leds_pixel_rgb leds_pixel_rgb(struct leds_color color, unsigned index, const struct leds_limit *limit)
{
    return (union leds_pixel_rgb) {
        .b  = leds_limit_uint8(limit, index, color.b),
        .g  = leds_limit_uint8(limit, index, color.g),
        .r  = leds_limit_uint8(limit, index, color.r),
    };
}

union leds_pixel_grb {
    struct {
      uint8_t b, r, g;
    };
  
    // aligned with 0xXXGGRRBB on little-endian architectures
    uint32_t _grb;
};

static inline union leds_pixel_grb leds_pixel_grb(struct leds_color color, unsigned index, const struct leds_limit *limit)
{
    return (union leds_pixel_grb) {
        .b  = leds_limit_uint8(limit, index, color.b),
        .r  = leds_limit_uint8(limit, index, color.r),
        .g  = leds_limit_uint8(limit, index, color.g),
    };
}
