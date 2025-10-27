#include "leds.h"

#include <logging.h>

unsigned leds_format_count(size_t len, enum leds_format format, unsigned group)
{
  if (!group) {
    group = 1;
  }

  switch (format) {
    case LEDS_FORMAT_RGB:
    case LEDS_FORMAT_BGR:
    case LEDS_FORMAT_GRB:
      return len / (3 * group) * group;

    case LEDS_FORMAT_RGBA:
    case LEDS_FORMAT_RGBW:
      return len / (4 * group) * group;

    case LEDS_FORMAT_RGBXI:
    case LEDS_FORMAT_BGRXI:
    case LEDS_FORMAT_GRBXI:
      return len / (3 + group) * group;

    case LEDS_FORMAT_RGBWXI:
      return len / (4 + group) * group;

    case LEDS_FORMAT_RGBXXI:
      return (len - 3 * group) * group;

    default:
      LOG_FATAL("invalid format=%d", format);
  }
}

static inline void set_leds_pixels(struct leds *leds, unsigned i, struct leds_format_params params, struct leds_color color)
{
  for (unsigned j = 0; j < params.segment; j++) {
    leds->pixels[params.index + i * params.segment + j] = color;
  }
}

void leds_set_format_rgb(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  uint8_t parameter = leds_parameter_default(leds);

  LOG_DEBUG("len=%u index=%u count=%u segment=%u", len, params.index, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 3; i++) {
    set_leds_pixels(leds, i, params, (struct leds_color) {
      .r = data[i * 3 + 0],
      .g = data[i * 3 + 1],
      .b = data[i * 3 + 2],

      .parameter = parameter,
    });
  }
}

void leds_set_format_bgr(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  uint8_t parameter = leds_parameter_default(leds);

  LOG_DEBUG("len=%u index=%u count=%u segment=%u", len, params.index, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 3; i++) {
    set_leds_pixels(leds, i, params, (struct leds_color) {
      .b = data[i * 3 + 0],
      .g = data[i * 3 + 1],
      .r = data[i * 3 + 2],

      .parameter = parameter,
    });
  }
}

void leds_set_format_grb(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  uint8_t parameter = leds_parameter_default(leds);

  LOG_DEBUG("len=%u index=%u count=%u segment=%u", len, params.index, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 3; i++) {
    set_leds_pixels(leds, i, params, (struct leds_color) {
      .g = data[i * 3 + 0],
      .r = data[i * 3 + 1],
      .b = data[i * 3 + 2],

      .parameter = parameter,
    });
  }
}

void leds_set_format_rgba(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  enum leds_parameter_type parameter = leds_parameter_type(leds);
  uint8_t parameter_default = leds_parameter_default(leds);

  LOG_DEBUG("len=%u index=%u count=%u segment=%u", len, params.index, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 4; i++) {
    set_leds_pixels(leds, i, params, (struct leds_color) {
      .r = data[i * 4 + 0],
      .g = data[i * 4 + 1],
      .b = data[i * 4 + 2],

      .dimmer = (parameter == LEDS_PARAMETER_DIMMER) ? data[i * 4 + 3] : parameter_default,
    });
  }
}

void leds_set_format_rgbw(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  enum leds_parameter_type parameter = leds_parameter_type(leds);
  uint8_t parameter_default = leds_parameter_default(leds);

  LOG_DEBUG("len=%u index=%u count=%u segment=%u", len, params.index, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 4; i++) {
    set_leds_pixels(leds, i, params, (struct leds_color) {
      .r = data[i * 4 + 0],
      .g = data[i * 4 + 1],
      .b = data[i * 4 + 2],

      .white = (parameter == LEDS_PARAMETER_WHITE) ? data[i * 4 + 3] : parameter_default,
    });
  }
}

void leds_set_format_rgbxi(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  enum leds_parameter_type parameter_type = leds_parameter_type(leds);
  uint8_t parameter_default = leds_parameter_default(leds);

  LOG_DEBUG("len=%u index=%u count=%u segment=%u group=%u", len, params.index, params.count, params.segment, params.group);

  size_t off = 0;

  for (unsigned g = 0; g * params.group < params.count && len >= off + 3 + params.group; g++) {
    struct leds_color group_color = {};

    group_color.r = data[off++];
    group_color.g = data[off++];
    group_color.b = data[off++];
    group_color.parameter = parameter_default;

    LOG_DEBUG("\tg=%u off=%u rgb=%02x%02x%02x", g, off, group_color.r, group_color.g, group_color.b);

    for (unsigned i = 0; i < params.group && g * params.group + i < params.count; i++) {
      uint8_t intensity = data[off++];
      struct leds_color pixel_color = leds_color_intensity(group_color, parameter_type, intensity);

      set_leds_pixels(leds, (g * params.group + i), params, pixel_color);
    }
  }
}

void leds_set_format_bgrxi(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  enum leds_parameter_type parameter_type = leds_parameter_type(leds);
  uint8_t parameter_default = leds_parameter_default(leds);

  LOG_DEBUG("len=%u index=%u count=%u segment=%u group=%u", len, params.index, params.count, params.segment, params.group);

  size_t off = 0;

  for (unsigned g = 0; g * params.group < params.count && len >= off + 3 + params.group; g++) {
    struct leds_color group_color = {};

    group_color.b = data[off++];
    group_color.g = data[off++];
    group_color.r = data[off++];
    group_color.parameter = parameter_default;

    LOG_DEBUG("\tg=%u off=%u rgb=%02x%02x%02x", g, off, group_color.r, group_color.g, group_color.b);

    for (unsigned i = 0; i < params.group && g * params.group + i < params.count; i++) {
      uint8_t intensity = data[off++];
      struct leds_color pixel_color = leds_color_intensity(group_color, parameter_type, intensity);

      set_leds_pixels(leds, (g * params.group + i), params, pixel_color);
    }
  }
}

void leds_set_format_grbxi(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  enum leds_parameter_type parameter_type = leds_parameter_type(leds);
  uint8_t parameter_default = leds_parameter_default(leds);

  LOG_DEBUG("len=%u index=%u count=%u segment=%u group=%u", len, params.index, params.count, params.segment, params.group);

  size_t off = 0;

  for (unsigned g = 0; g * params.group < params.count && len >= off + 3 + params.group; g++) {
    struct leds_color group_color = {};

    group_color.g = data[off++];
    group_color.r = data[off++];
    group_color.b = data[off++];
    group_color.parameter = parameter_default;

    LOG_DEBUG("\tg=%u off=%u rgb=%02x%02x%02x", g, off, group_color.r, group_color.g, group_color.b);

    for (unsigned i = 0; i < params.group && g * params.group + i < params.count; i++) {
      uint8_t intensity = data[off++];
      struct leds_color pixel_color = leds_color_intensity(group_color, parameter_type, intensity);

      set_leds_pixels(leds, (g * params.group + i), params, pixel_color);
    }
  }
}

void leds_set_format_rgbwxi(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  enum leds_parameter_type parameter_type = leds_parameter_type(leds);

  LOG_DEBUG("len=%u index=%u count=%u segment=%u group=%u", len, params.index, params.count, params.segment, params.group);

  size_t off = 0;

  for (unsigned g = 0; g * params.group < params.count && len >= off + 4 + params.group; g++) {
    struct leds_color group_color = {};

    group_color.r = data[off++];
    group_color.g = data[off++];
    group_color.b = data[off++];
    group_color.w = data[off++];

    LOG_DEBUG("\tg=%u off=%u rgbw=%02x%02x%02x%02x", g, off, group_color.r, group_color.g, group_color.b, group_color.w);

    for (unsigned i = 0; i < params.group && g * params.group + i < params.count; i++) {
      uint8_t intensity = data[off++];
      struct leds_color pixel_color = leds_color_intensity(group_color, parameter_type, intensity);

      set_leds_pixels(leds, (g * params.group + i), params, pixel_color);
    }
  }
}

void leds_set_format_rgbxxi(struct leds *leds, const uint8_t *data, size_t len, struct leds_format_params params)
{
  enum leds_parameter_type parameter_type = leds_parameter_type(leds);
  uint8_t parameter_default = leds_parameter_default(leds);

  LOG_DEBUG("len=%u index=%u count=%u segment=%u group=%u", len, params.index, params.count, params.segment, params.group);

  for (unsigned i = 0; i * params.group < params.count && 3 * params.group + i < len; i++) {
    uint8_t intensity = data[3 * params.group + i];

    for (unsigned j = 0; j < params.group && i * params.group + j < params.count; j++) {
      struct leds_color pixel_color = {
        .r = data[j * 3 + 0],
        .g = data[j * 3 + 1],
        .b = data[j * 3 + 2],

        .parameter = parameter_default,
      };

      pixel_color = leds_color_intensity(pixel_color, parameter_type, intensity);

      set_leds_pixels(leds, i * params.group + j, params, pixel_color);
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

  if (params.index > leds->options.count) {
    LOG_DEBUG("index=%u is over options.count=%u", params.index, leds->options.count);
    params.count = 0;
  } else if (params.index + (params.count * params.segment) > leds->options.count) {
    LOG_DEBUG("index=%u + count=%u * segment=%u is over options.count=%u", params.index, params.count, params.segment, leds->options.count);
    params.count = (leds->options.count - params.index) / params.segment;
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

    case LEDS_FORMAT_RGBXI:
      leds_set_format_rgbxi(leds, data, len, params);
      return 0;

    case LEDS_FORMAT_BGRXI:
      leds_set_format_bgrxi(leds, data, len, params);
      return 0;

    case LEDS_FORMAT_GRBXI:
      leds_set_format_grbxi(leds, data, len, params);
      return 0;

    case LEDS_FORMAT_RGBWXI:
      leds_set_format_rgbwxi(leds, data, len, params);
      return 0;

    case LEDS_FORMAT_RGBXXI:
      leds_set_format_rgbxxi(leds, data, len, params);
      return 0;

    default:
      LOG_ERROR("unknown format=%#x", format);
      return -1;
  }
}
