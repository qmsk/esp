#include "leds.h"
#include "leds_state.h"
#include "leds_test.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>
#include <json.h>

#include <string.h>

/* GET /api/leds/test */
static int leds_api_write_test_array(struct json_writer *w)
{
  int err;

  for (const struct config_enum *e = leds_test_mode_enum; e->name; e++) {
    if ((err = JSON_WRITE_OBJECT(w,
      JSON_WRITE_MEMBER_STRING(w, "mode", e->name)
    ))) {
      return err;
    }
  }

  return 0;
}

static int leds_api_write_test(struct json_writer *w, void *ctx)
{
  return JSON_WRITE_ARRAY(w, leds_api_write_test_array(w));
}

int leds_api_test_get(struct http_request *request, struct http_response *response, void *ctx)
{
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = write_http_response_json(response, leds_api_write_test, NULL))) {
    LOG_WARN("write_http_response_json -> leds_api_write_test");
    return err;
  }

  return 0;
}


/* POST /api/leds/test */
struct leds_api_test_params {
  struct leds_state *state;
  enum leds_test_mode mode;
  bool auto_mode;
};

int leds_api_test_params_set(struct leds_api_test_params *params, const char *key, const char *value)
{
  if (strcmp(key, "index") == 0) {
    unsigned index;

    if (sscanf(value, "%d", &index) <= 0) {
      return HTTP_UNPROCESSABLE_ENTITY;
    } else if (index <= 0 || index > LEDS_COUNT) {
      return HTTP_UNPROCESSABLE_ENTITY;
    } else {
      params->state = &leds_states[index - 1];
    }
  } else if (strcmp(key, "mode") == 0) {
    int mode;

    if ((mode = config_enum_to_value(leds_test_mode_enum, value)) < 0) {
      return HTTP_UNPROCESSABLE_ENTITY;
    } else {
      params->mode = mode;
    }
  } else if (strcmp(key, "auto") == 0) {
    if (strcmp(value, "true") == 0) {
      params->auto_mode = true;
    } else if (strcmp(value, "false") == 0) {
      params->auto_mode = false;
    } else {
      return HTTP_UNPROCESSABLE_ENTITY;
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

  // TODO: clear?
  if (!params.state) {
    LOG_WARN("missing index");
    return HTTP_UNPROCESSABLE_ENTITY;
  } else if ((err = set_leds_test(state, params.mode, params.auto_mode)) < 0) {
    LOG_ERROR("set_leds_test");
    return HTTP_INTERNAL_SERVER_ERROR;
  } else if (err) {
    LOG_WARN("set_leds_test");
    return HTTP_CONFLICT;
  }

  return HTTP_NO_CONTENT;
}
