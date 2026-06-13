#include "leds.h"
#include "leds_api.h"
#include "leds_config.h"
#include "leds_state.h"
#include "leds_task.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>
#include <json.h>

#include <string.h>

struct leds_api_params {
  struct leds_state *state;
};

int leds_api_state_parse(struct leds_api_params *params, const char *key, const char *value)
{
  struct leds *leds = NULL;
  enum leds_parameter_type parameter_type = 0;
  struct leds_color color;
  unsigned index;
  int ret;

  if (strcmp(key, "leds") == 0) {
    if (params->state) {
      LOG_WARN("duplicate leds=");
      return HTTP_UNPROCESSABLE_ENTITY;
    } else if ((ret = leds_api_leds_parse(&params->state, value))) {
      LOG_WARN("leds_api_leds_parse");
      return ret;
    }
    
    if ((ret = start_leds_update(params->state, LEDS_UPDATE_HTTP))) {
      LOG_ERROR("start_leds_update");
      params->state = NULL;
      return ret;
    }
    
    return 0;

  } else if (!params->state) {
    LOG_WARN("missing leds= in request");
    return HTTP_UNPROCESSABLE_ENTITY;

  } else {
    leds = params->state->leds;
    parameter_type = leds_parameter_type(params->state->leds);
  }

  if (strcmp(key, "all") == 0) {
    if ((ret = leds_api_color_parse(&color, parameter_type, value))) {
      return ret;
    }

    leds_set_all(leds, color);

    return 0;

  } else if (sscanf(key, "%u", &index) > 0) {
    if ((ret = leds_api_color_parse(&color, parameter_type, value))) {
      return ret;
    }

    return leds_set(leds, index, color);

  } else {
    return HTTP_UNPROCESSABLE_ENTITY;
  }
}

int leds_api_form(struct http_request *request, struct http_response *response)
{
  struct leds_api_params params = {};
  char *key, *value;
  int err;

  // leds_api_state_parse() will implicitly start_leds_update(), we must end_leds_update()
  while (!(err = http_request_form(request, &key, &value))) {
    if ((err = leds_api_state_parse(&params, key, value)) < 0) {
      LOG_ERROR("leds_api_state_parse");
      goto error;
    } else if (err) {
      LOG_WARN("leds_api_state_parse: %s=%s -> %d", key, value ? value : "", err);
      goto error;
    }
  }

  if (err < 0) {
    LOG_ERROR("http_request_form");
    return err;
  }

  err = HTTP_NO_CONTENT;

error:
  if (params.state) {
    end_leds_update(params.state);
  }

  return err;
}

int leds_api_post(struct http_request *request, struct http_response *response, void *ctx)
{
  const struct http_request_headers *headers;
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  switch (headers->content_type) {
    case HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED:
      return leds_api_form(request, response);

    default:
      LOG_WARN("Unknown Content-Type");

      return HTTP_UNSUPPORTED_MEDIA_TYPE;
  }
}
