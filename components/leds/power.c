#include <leds.h>
#include "leds.h"

#include <logging.h>

static inline unsigned leds_power_rgb(struct leds_color color)
{
  return color.r + color.g + color.b;
}

static inline unsigned leds_power_rgba(struct leds_color color)
{
  // use brightness to 0..31 to not overflow a 32-bit uint for a 16-bit LEDS_COUNT_MAX
  return (color.r + color.g + color.b) * (color.dimmer >> 3);
}

static inline unsigned leds_power_rgbw(struct leds_color color)
{
  return color.r + color.g + color.b + color.white;
}

static inline unsigned leds_power_rgb2w(struct leds_color color)
{
  // white channel uses 200% power
  return color.r + color.g + color.b + (2 * color.white);
}

unsigned leds_power_total(const struct leds_color *pixels, unsigned index, unsigned count, enum leds_power_mode power_mode)
{
  unsigned power = 0;

  for (unsigned i = index; i < index + count; i++) {
    switch (power_mode) {
      case LEDS_POWER_NONE:
        break;

      case LEDS_POWER_RGB:
        power += leds_power_rgb(pixels[i]);
        break;

      case LEDS_POWER_RGBA:
        power += leds_power_rgba(pixels[i]);
        break;

      case LEDS_POWER_RGBW:
        power += leds_power_rgbw(pixels[i]);
        break;

      case LEDS_POWER_RGB2W:
        power += leds_power_rgb2w(pixels[i]);
        break;
    }
  }

  switch (power_mode) {
    case LEDS_POWER_NONE:
      return 0;

    case LEDS_POWER_RGB:
      return power / (3 * 255);

    case LEDS_POWER_RGBA:
      return power / (3 * 255 * 31);

    case LEDS_POWER_RGBW:
      return power / (4 * 255);

    case LEDS_POWER_RGB2W:
      return power / (5 * 255);

    default:
      LOG_FATAL("invalid power_mode=%d", power_mode);
  }
}
