#include "sk9822.h"
#include "../leds.h"
#include "../interfaces/i2s.h"

#include <logging.h>

#include <stdlib.h>

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

static inline union sk9822_pixel sk9822_pixel(struct leds_color color, unsigned index, const struct leds_limit *limit)
{
  // TODO: use driving current instead of PWM for power limit?
  return (union sk9822_pixel) {
    .global = SK9822_GLOBAL_BYTE(color.dimmer),
    .b      = leds_limit_uint8(limit, index, color.b),
    .g      = leds_limit_uint8(limit, index, color.g),
    .r      = leds_limit_uint8(limit, index, color.r),
  };
}


#if CONFIG_LEDS_I2S_ENABLED
  #define LEDS_PROTOCOL_SK9822_INTERFACE_I2S_MODE LEDS_INTERFACE_I2S_MODE_32BIT_BCK

  static void leds_protocol_sk9822_i2s_out(uint32_t buf[1], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit)
  {
    union sk9822_pixel pixel = sk9822_pixel(pixels[index], index, limit);

    // 32-bit little-endian
    buf[0] = pixel.xbgr;
  }
#endif

int leds_protocol_sk9822_init(union leds_interface_state *interface, const struct leds_options *options)
{
  int err;

  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      break;

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      if ((err = leds_interface_i2s_init(&interface->i2s, &options->i2s, LEDS_PROTOCOL_SK9822_INTERFACE_I2S_MODE, LEDS_INTERFACE_I2S_FUNC(i2s_mode_32bit, leds_protocol_sk9822_i2s_out)))) {
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

int leds_protocol_sk9822_tx(union leds_interface_state *interface, const struct leds_options *options, const struct leds_color *pixels, const struct leds_limit *limit)
{
  switch (options->interface) {
    case LEDS_INTERFACE_NONE:
      return 0;

  #if CONFIG_LEDS_I2S_ENABLED
    case LEDS_INTERFACE_I2S:
      // TODO: SK9822_START_FRAME_UINT32
      return leds_interface_i2s_tx(&interface->i2s, pixels, options->count, limit);
  #endif

    default:
      LOG_ERROR("unsupported interface=%#x", options->interface);
      return -1;
  }
}

struct leds_protocol_type leds_protocol_sk9822 = {
#if CONFIG_LEDS_I2S_ENABLED
  .i2s_interface_mode  = LEDS_PROTOCOL_SK9822_INTERFACE_I2S_MODE,
#endif
  .parameter_type     = LEDS_PARAMETER_DIMMER,
  .power_mode         = LEDS_POWER_RGBA,

  .init               = &leds_protocol_sk9822_init,
  .tx                 = &leds_protocol_sk9822_tx,
};
