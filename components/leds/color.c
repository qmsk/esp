#include <leds.h>

#include <logging.h>

static inline uint8_t u8max(uint8_t a, uint8_t b)
{
  return a > b ? a : b;
}

uint8_t leds_parameter_default_for_type(enum leds_parameter_type parameter_type)
{
  switch (parameter_type) {
    case LEDS_PARAMETER_NONE:
      return 0;

    case LEDS_PARAMETER_DIMMER:
      return 255;

    case LEDS_PARAMETER_WHITE:
      return 0;

    default:
      // unknown
      return 0;
  }
}

bool leds_color_active (struct leds_color color, enum leds_parameter_type parameter_type)
{
  switch (parameter_type) {
    case LEDS_PARAMETER_NONE:
      return color.r || color.g || color.b;

    case LEDS_PARAMETER_DIMMER:
      return (color.r || color.g || color.b) && color.dimmer;

    case LEDS_PARAMETER_WHITE:
      return color.r || color.g || color.b || color.white;

    default:
      LOG_FATAL("invalid parameter_type=%u", parameter_type);
  }
}

unsigned leds_colors_active (const struct leds_color *pixels, unsigned count, enum leds_parameter_type parameter_type)
{
  unsigned active = 0;

  for (unsigned i = 0; i < count; i++) {
    if (leds_color_active(pixels[i], parameter_type)) {
      active++;
    }
  }

  return active;
}

struct leds_color leds_color_intensity (struct leds_color color, enum leds_parameter_type parameter_type, uint8_t intensity)
{
  switch (parameter_type) {
    case LEDS_PARAMETER_DIMMER:
      color.dimmer = intensity;
      break;

    case LEDS_PARAMETER_NONE:
      color.r = color.r * intensity / 255;
      color.g = color.g * intensity / 255;
      color.b = color.b * intensity / 255;
      color.parameter = 0;
      break;

    case LEDS_PARAMETER_WHITE:
      color.r = color.r * intensity / 255;
      color.g = color.g * intensity / 255;
      color.b = color.b * intensity / 255;
      color.w = color.w * intensity / 255;
      break;
  }

  return color;
}

struct leds_color leds_color_max (struct leds_color c1, struct leds_color c2, enum leds_parameter_type parameter_type)
{
  switch (parameter_type) {
    case LEDS_PARAMETER_NONE:
      return (struct leds_color) {
        .r  = u8max(c1.r, c2.r),
        .g  = u8max(c1.g, c2.g),
        .b  = u8max(c1.b, c2.b),
      };

    case LEDS_PARAMETER_DIMMER:
      return (struct leds_color) {
        .r  = u8max(c1.r, c2.r),
        .g  = u8max(c1.g, c2.g),
        .b  = u8max(c1.b, c2.b),

        .dimmer  = u8max(c1.dimmer, c2.dimmer), // TODO: what makes the most sense?
      };

    case LEDS_PARAMETER_WHITE:
      return (struct leds_color) {
        .r  = u8max(c1.r, c2.r),
        .g  = u8max(c1.g, c2.g),
        .b  = u8max(c1.b, c2.b),
        .w  = u8max(c1.w, c2.w),
      };
    
    default:
      LOG_FATAL("%d", parameter_type);
  }
}
