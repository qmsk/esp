#include "leds.h"
#include "leds_api.h"
#include "leds_config.h"
#include "leds_state.h"
#include "leds_static.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <config.h>
#include <logging.h>
#include <json.h>

#include <string.h>

/* POST /api/leds/static */
struct leds_api_static_params {
  struct leds_state *state;
  struct leds_color color;
};

int leds_api_static_params_set(struct leds_api_static_params *params, const char *key, const char *value)
{
  if (strcmp(key, "leds") == 0) {
    return leds_api_leds_parse(&params->state, value);
  } else if (strcmp(key, "color") == 0) {
    if (!params->state) {
      return HTTP_UNPROCESSABLE_ENTITY;
    } else if (leds_api_color_parse(&params->color, leds_parameter_type_for_protocol(leds_protocol(params->state->leds)), value)) {
      return HTTP_UNPROCESSABLE_ENTITY;
    }
  } else {
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  return 0;
}

int leds_api_static_read_form_params(struct http_request *request, struct leds_api_static_params *params)
{
  char *key, *value;
  int err;

  while (!(err = http_request_form(request, &key, &value))) {
    if ((err = leds_api_static_params_set(params, key, value))) {
      LOG_WARN("leds_api_static_params_set: %s=%s", key, value ? value : "");
      return err;
    }
  }

  if (err < 0) {
    LOG_ERROR("http_request_form");
    return err;
  }

  if (!params->state) {
    LOG_WARN("missing leds=");
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  return 0;
}

int leds_api_static_post(struct http_request *request, struct http_response *response, void *ctx)
{
  const struct http_request_headers *headers;
  struct leds_api_static_params params = {

  };
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  switch (headers->content_type) {
    case HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED:
      if ((err = leds_api_static_read_form_params(request, &params))) {
        LOG_WARN("leds_api_static_read_form_params");
        return err;
      }

      break;

    default:
      LOG_WARN("Unknown Content-Type");

      return HTTP_UNSUPPORTED_MEDIA_TYPE;
  }

  if ((err = set_leds_static(params.state, params.color)) < 0) {
    LOG_ERROR("set_leds_static");
    return HTTP_INTERNAL_SERVER_ERROR;
  } else {
    return HTTP_NO_CONTENT;
  }
}
