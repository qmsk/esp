#include "leds.h"
#include "leds_config.h"
#include "leds_state.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>
#include <json.h>

#include <string.h>

struct leds_api_req {
  struct leds_state *state;
};

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

int leds_api_state_parse(struct leds_api_req *req, const char *key, const char *value)
{
  struct leds *leds = NULL;
  enum leds_parameter_type parameter_type = 0;
  struct leds_color color;
  unsigned index;
  int ret;

  // XXX: will only update last state
  if (strcmp(key, "id") == 0) {
    if (sscanf(value, "%d", &index) <= 0) {
      return HTTP_UNPROCESSABLE_ENTITY;
    } else if (index <= 0 || index > LEDS_COUNT) {
      return HTTP_UNPROCESSABLE_ENTITY;
    } else {
      req->state = &leds_states[index - 1];
    }

    return 0;

  } else if (!req->state) {
    LOG_WARN("missing id= in request");
    return HTTP_UNPROCESSABLE_ENTITY;

  } else if (!req->state->leds) {
    LOG_WARN("disabled id= in request");
    return HTTP_UNPROCESSABLE_ENTITY;
  } else {
    leds = req->state->leds;
    parameter_type = leds_parameter_type(req->state->leds);
  }

  if (strcmp(key, "all") == 0) {
    if ((ret = leds_api_color_parse(&color, parameter_type, value))) {
      return ret;
    }

    return leds_set_all(leds, color);

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
  struct leds_api_req req = {};
  char *key, *value;
  int err;

  while (!(err = http_request_form(request, &key, &value))) {
    if ((err = leds_api_state_parse(&req, key, value)) < 0) {
      LOG_ERROR("leds_api_state_parse");
      return err;
    } else if (err) {
      LOG_WARN("leds_api_state_parse: %s=%s", key, value ? value : "");
      return err;
    }
  }

  if (err < 0) {
    LOG_ERROR("http_request_form");
    return err;
  }

  if (req.state && req.state->leds) {
    if ((err = update_leds(req.state)) < 0) {
      LOG_ERROR("update_leds");
      return HTTP_INTERNAL_SERVER_ERROR;
    } else if (err) {
      LOG_WARN("update_leds");
      return HTTP_CONFLICT;
    }
  }

  return HTTP_NO_CONTENT;
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

/* POST /api/leds/test */
struct leds_api_test_params {
  int index;
  enum leds_test_mode mode;
};

int leds_api_test_params_set(struct leds_api_test_params *params, const char *key, const char *value)
{
  if (strcmp(key, "index") == 0) {
    if (sscanf(value, "%d", &params->index) <= 0) {
      return HTTP_UNPROCESSABLE_ENTITY;
    }
  } else if (strcmp(key, "mode") == 0) {
    int mode;

    if ((mode = config_enum_to_value(leds_test_mode_enum, value)) < 0) {
      return HTTP_UNPROCESSABLE_ENTITY;
    } else {
      params->mode = mode;
    }
  } else {
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  return 0;
}

/* POST /api/config application/x-www-form-urlencoded */
int leds_api_test_read_form_params(struct http_request *request, struct leds_api_test_params *params)
{
  char *key, *value;
  int err;

  while (!(err = http_request_form(request, &key, &value))) {
    if ((err = leds_api_test_params_set(params, key, value))) {
      LOG_WARN("leds_api_test_params_set: %s=%s", key, value ? value : "");
      return err;
    }
  }

  if (err < 0) {
    LOG_ERROR("http_request_form");
    return err;
  }

  return 0;
}

int leds_api_test_post(struct http_request *request, struct http_response *response, void *ctx)
{
  const struct http_request_headers *headers;
  struct leds_api_test_params params = {
    .mode = TEST_MODE_CHASE,
  };
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  switch (headers->content_type) {
    case HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED:
      if ((err = leds_api_test_read_form_params(request, &params))) {
        LOG_WARN("leds_api_test_read_form_params");
        return err;
      }

      break;

    default:
      LOG_WARN("Unknown Content-Type");

      return HTTP_UNSUPPORTED_MEDIA_TYPE;
  }

  // decode
  struct leds_state *state;

  if (params.index <= 0 || params.index > LEDS_COUNT) {
    LOG_WARN("invalid index=%d", params.index);
    return HTTP_UNPROCESSABLE_ENTITY;
  } else {
    state = &leds_states[params.index - 1];
  }

  // XXX: may block for some time
  if ((err = test_leds_mode(state, params.mode)) < 0) {
    LOG_ERROR("test_leds");
    return HTTP_INTERNAL_SERVER_ERROR;
  } else if (err) {
    LOG_WARN("test_leds");
    return HTTP_CONFLICT;
  }

  return HTTP_NO_CONTENT;
}
