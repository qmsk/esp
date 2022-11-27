#include "leds.h"

#include <logging.h>

void leds_set_format_rgb(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params)
{
  uint8_t parameter = leds_parameter_default(leds);

  LOG_DEBUG("len=%u offset=%u count=%u segment=%u", len, params.offset, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 3; i++) {
    for (unsigned j = 0; j < params.segment; j++) {
      leds_set(leds, params.offset + i * params.segment + j, (struct leds_color) {
        .r = data[i * 3 + 0],
        .g = data[i * 3 + 1],
        .b = data[i * 3 + 2],

        .parameter = parameter,
      });
    }
  }
}

void leds_set_format_bgr(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params)
{
  uint8_t parameter = leds_parameter_default(leds);

  LOG_DEBUG("len=%u offset=%u count=%u segment=%u", len, params.offset, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 3; i++) {
    for (unsigned j = 0; j < params.segment; j++) {
      leds_set(leds, params.offset + i * params.segment + j, (struct leds_color) {
        .b = data[i * 3 + 0],
        .g = data[i * 3 + 1],
        .r = data[i * 3 + 2],

        .parameter = parameter,
      });
    }
  }
}

void leds_set_format_grb(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params)
{
  uint8_t parameter = leds_parameter_default(leds);

  LOG_DEBUG("len=%u offset=%u count=%u segment=%u", len, params.offset, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 3; i++) {
    for (unsigned j = 0; j < params.segment; j++) {
      leds_set(leds, params.offset + i * params.segment + j, (struct leds_color) {
        .g = data[i * 3 + 0],
        .r = data[i * 3 + 1],
        .b = data[i * 3 + 2],

        .parameter = parameter,
      });
    }
  }
}

void leds_set_format_rgba(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params)
{
  enum leds_parameter_type parameter = leds_parameter_type(leds);
  uint8_t parameter_default = leds_parameter_default(leds);

  LOG_DEBUG("len=%u offset=%u count=%u segment=%u", len, params.offset, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 4; i++) {
    for (unsigned j = 0; j < params.segment; j++) {
      leds_set(leds, params.offset + i * params.segment + j, (struct leds_color) {
        .r = data[i * 4 + 0],
        .g = data[i * 4 + 1],
        .b = data[i * 4 + 2],

        .dimmer = (parameter == LEDS_PARAMETER_DIMMER) ? data[i * 4 + 3] : parameter_default,
      });
    }
  }
}

void leds_set_format_rgbw(struct leds *leds, uint8_t *data, size_t len, struct leds_format_params params)
{
  enum leds_parameter_type parameter = leds_parameter_type(leds);
  uint8_t parameter_default = leds_parameter_default(leds);

  LOG_DEBUG("len=%u offset=%u count=%u segment=%u", len, params.offset, params.count, params.segment);

  for (unsigned i = 0; i < params.count && len >= (i + 1) * 4; i++) {
    for (unsigned j = 0; j < params.segment; j++) {
      leds_set(leds, params.offset + i * params.segment + j, (struct leds_color) {
        .r = data[i * 4 + 0],
        .g = data[i * 4 + 1],
        .b = data[i * 4 + 2],

        .white = (parameter == LEDS_PARAMETER_WHITE) ? data[i * 4 + 3] : parameter_default,
      });
    }
  }
}
