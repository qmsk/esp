#include "leds_api.h"

#include <logging.h>
#include <http/http_types.h>
#include <string.h>

int leds_api_leds_parse(struct leds_state **statep, const char *value)
{
  struct leds_state *state;
  int index;

  if (sscanf(value, "leds%d", &index) <= 0) {
    LOG_WARN("invalid leds=%s", value);
    return HTTP_UNPROCESSABLE_ENTITY;
  } else if (index <= 0 || index > LEDS_COUNT) {
    LOG_WARN("invalid leds=%s index", value);
    return HTTP_UNPROCESSABLE_ENTITY;
  } else {
    state = &leds_states[index - 1];
  }

  if (!state || !state->leds) {
    LOG_WARN("disabled leds=%s", value);
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  *statep = state;

  return 0;
}

int leds_api_write_color(struct json_writer *w, struct leds_color c, enum leds_parameter_type parameter_type)
{
  switch (parameter_type) {
    case LEDS_PARAMETER_NONE:
      return json_write_raw(w, "\"%02x%02x%02x\"", c.r, c.g, c.b);

    case LEDS_PARAMETER_DIMMER:
    case LEDS_PARAMETER_WHITE:
      return json_write_raw(w, "\"%02x%02x%02x.%02x\"", c.r, c.g, c.b, c.parameter);

    default:
      LOG_FATAL("%d", parameter_type);
  }
}

int leds_api_color_parse(struct leds_color *color, enum leds_parameter_type parameter_type, const char *value)
{
  int rgb;
  int parameter = leds_parameter_default_for_type(parameter_type);

  if (!value) {
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  if (sscanf(value, "%x.%x", &rgb, &parameter) <= 0) {
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  if (parameter < 0 || parameter > UINT8_MAX) {
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  color->r = (rgb >> 16) & 0xFF;
  color->g = (rgb >>  8) & 0xFF;
  color->b = (rgb >>  0) & 0xFF;
  color->parameter = parameter;

  return 0;
}
