#include "leds.h"

#include <logging.h>

unsigned leds_format_count(enum leds_format format, size_t len, size_t group)
{
  if (group == 0) {
    group = 1;
  }

  switch (format) {
    case LEDS_FORMAT_RGB:
    case LEDS_FORMAT_BGR:
    case LEDS_FORMAT_GRB:
      return len / 3;

    case LEDS_FORMAT_RGBA:
    case LEDS_FORMAT_RGBW:
      return len / 4;

    case LEDS_FORMAT_RGBWI:
      return len / (4 + group) * group;

    default:
      LOG_FATAL("invalid format=%d", format);
  }
}

void leds_set_format_rgb(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  uint8_t parameter = leds_parameter_default(leds);

  LOG_DEBUG("len=%u offset=%u count=%u segment=%u", len, params.offset, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 3; i++) {
    for (unsigned j = 0; j < params.segment; j++) {
      leds->pixels[params.offset + i * params.segment + j] = (struct leds_color) {
        .r = data[i * 3 + 0],
        .g = data[i * 3 + 1],
        .b = data[i * 3 + 2],

        .parameter = parameter,
      };
    }
  }
}

void leds_set_format_bgr(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  uint8_t parameter = leds_parameter_default(leds);

  LOG_DEBUG("len=%u offset=%u count=%u segment=%u", len, params.offset, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 3; i++) {
    for (unsigned j = 0; j < params.segment; j++) {
      leds->pixels[params.offset + i * params.segment + j] = (struct leds_color) {
        .b = data[i * 3 + 0],
        .g = data[i * 3 + 1],
        .r = data[i * 3 + 2],

        .parameter = parameter,
      };
    }
  }
}

void leds_set_format_grb(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  uint8_t parameter = leds_parameter_default(leds);

  LOG_DEBUG("len=%u offset=%u count=%u segment=%u", len, params.offset, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 3; i++) {
    for (unsigned j = 0; j < params.segment; j++) {
      leds->pixels[params.offset + i * params.segment + j] = (struct leds_color) {
        .g = data[i * 3 + 0],
        .r = data[i * 3 + 1],
        .b = data[i * 3 + 2],

        .parameter = parameter,
      };
    }
  }
}

void leds_set_format_rgba(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  enum leds_parameter_type parameter = leds_parameter_type(leds);
  uint8_t parameter_default = leds_parameter_default(leds);

  LOG_DEBUG("len=%u offset=%u count=%u segment=%u", len, params.offset, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 4; i++) {
    for (unsigned j = 0; j < params.segment; j++) {
      leds->pixels[params.offset + i * params.segment + j] = (struct leds_color) {
        .r = data[i * 4 + 0],
        .g = data[i * 4 + 1],
        .b = data[i * 4 + 2],

        .dimmer = (parameter == LEDS_PARAMETER_DIMMER) ? data[i * 4 + 3] : parameter_default,
      };
    }
  }
}

void leds_set_format_rgbw(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  enum leds_parameter_type parameter = leds_parameter_type(leds);
  uint8_t parameter_default = leds_parameter_default(leds);

  LOG_DEBUG("len=%u offset=%u count=%u segment=%u", len, params.offset, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 4; i++) {
    for (unsigned j = 0; j < params.segment; j++) {
      leds->pixels[params.offset + i * params.segment + j] = (struct leds_color) {
        .r = data[i * 4 + 0],
        .g = data[i * 4 + 1],
        .b = data[i * 4 + 2],

        .white = (parameter == LEDS_PARAMETER_WHITE) ? data[i * 4 + 3] : parameter_default,
      };
    }
  }
}

void leds_set_format_rgbwi(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  enum leds_parameter_type parameter_type = leds_parameter_type(leds);

  LOG_DEBUG("len=%u offset=%u count=%u segment=%u group=%u", len, params.offset, params.count, params.segment, params.group);

  size_t off = 0;

  for (unsigned g = 0; g * params.group < params.count && len >= off + 4 + params.group; g++) {
    struct leds_color group_color;

    group_color.r = data[off++];
    group_color.g = data[off++];
    group_color.b = data[off++];
    group_color.w = data[off++];

    LOG_DEBUG("\tg=%u off=%u rgbw=%02x%02x%02x%02x", g, off, group_color.r, group_color.g, group_color.b, group_color.w);

    for (unsigned i = 0; i < params.group && g * params.group + i < params.count; i++) {
      uint8_t intensity = data[off++];
      struct leds_color pixel_color = leds_color_intensity(group_color, parameter_type, intensity);

      for (unsigned j = 0; j < params.segment; j++) {
        leds->pixels[params.offset + (g * params.group + i) * params.segment + j] = pixel_color;
      }
    }
  }
}

int leds_set_format(struct leds *leds, enum leds_format format, const void *data, size_t len, struct leds_format_params params)
{
  if (params.count == 0) {
    params.count = leds->options.count;
  }

  if (params.segment == 0) {
    params.segment = 1;
  }

  if (params.group == 0) {
    params.group = 1;
  }

  if (params.offset > leds->options.count) {
    LOG_DEBUG("offset=%u is over options.count=%u", params.offset, leds->options.count);
    params.count = 0;
  } else if (params.offset + (params.count * params.segment) > leds->options.count) {
    LOG_DEBUG("offset=%u + count=%u * segment=%u is over options.count=%u", params.offset, params.count, params.segment, leds->options.count);
    params.count = (leds->options.count - params.offset) / params.segment;
  }

  leds->pixels_limit_dirty = true;

  switch(format) {
    case LEDS_FORMAT_RGB:
      leds_set_format_rgb(leds, data, len, params);
      return 0;

    case LEDS_FORMAT_BGR:
      leds_set_format_bgr(leds, data, len, params);
      return 0;

    case LEDS_FORMAT_GRB:
      leds_set_format_grb(leds, data, len, params);
      return 0;

    case LEDS_FORMAT_RGBA:
      leds_set_format_rgba(leds, data, len, params);
      return 0;

    case LEDS_FORMAT_RGBW:
      leds_set_format_rgbw(leds, data, len, params);
      return 0;

    case LEDS_FORMAT_RGBWI:
      leds_set_format_rgbwi(leds, data, len, params);
      return 0;

    default:
      LOG_ERROR("unknown format=%#x", format);
      return -1;
  }
}
