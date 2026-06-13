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

struct leds_api_req {
  struct leds_state *state;
};


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
    if ((err = update_leds(req.state, USER_ACTIVITY_LEDS_HTTP)) < 0) {
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
