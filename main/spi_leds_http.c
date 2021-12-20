#include "spi_leds.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>
#include <json.h>

#include <string.h>

static int spi_leds_api_write_object_enabled(struct json_writer *w, int index, struct spi_leds_state *state)
{
  const struct spi_leds_options *options = spi_leds_options(state->spi_leds);

  return (
        JSON_WRITE_MEMBER_STRING(w, "interface", config_enum_to_string(spi_leds_interface_enum, options->interface))
    ||  JSON_WRITE_MEMBER_STRING(w, "protocol", config_enum_to_string(spi_leds_protocol_enum, options->protocol))
    ||  JSON_WRITE_MEMBER_UINT(w, "count", options->count)
    ||  JSON_WRITE_MEMBER_UINT(w, "active", state->active)
  );
}

static int spi_leds_api_write_object(struct json_writer *w, int id, struct spi_leds_state *state)
{
  bool enabled = state->config->enabled && state->spi_leds;

  return (
        JSON_WRITE_MEMBER_UINT(w, "id", id)
    ||  JSON_WRITE_MEMBER_BOOL(w, "enabled", enabled)
    ||  (enabled ? spi_leds_api_write_object_enabled(w, id, state) : 0)
  );
}

static int spi_leds_api_write_array(struct json_writer *w)
{
  int err;

  for (int i = 0; i < SPI_LEDS_COUNT; i++)
  {
    struct spi_leds_state *state = &spi_leds_states[i];

    if ((err = JSON_WRITE_OBJECT(w, spi_leds_api_write_object(w, i, state)))) {
      return err;
    }
  }

  return 0;
}

static int spi_leds_api_write(struct json_writer *w, void *ctx)
{
  return JSON_WRITE_ARRAY(w, spi_leds_api_write_array(w));
}

int spi_leds_api_get(struct http_request *request, struct http_response *response, void *ctx)
{
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = write_http_response_json(response, spi_leds_api_write, NULL))) {
    LOG_WARN("write_http_response_json -> spi_leds_api_write");
    return err;
  }

  return 0;
}

static int spi_leds_api_write_test_array(struct json_writer *w)
{
  int err;

  for (const struct config_enum *e = spi_leds_test_mode_enum; e->name; e++) {
    if ((err = JSON_WRITE_OBJECT(w,
      JSON_WRITE_MEMBER_STRING(w, "mode", e->name)
    ))) {
      return err;
    }
  }

  return 0;
}

static int spi_leds_api_write_test(struct json_writer *w, void *ctx)
{
  return JSON_WRITE_ARRAY(w, spi_leds_api_write_test_array(w));
}

int spi_leds_api_test_get(struct http_request *request, struct http_response *response, void *ctx)
{
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = write_http_response_json(response, spi_leds_api_write_test, NULL))) {
    LOG_WARN("write_http_response_json -> spi_leds_api_write_test");
    return err;
  }

  return 0;
}

/* POST /api/leds/test */
struct spi_leds_api_test_params {
  int id;
  enum spi_leds_test_mode mode;
};

int spi_leds_api_test_params_set(struct spi_leds_api_test_params *params, const char *key, const char *value)
{
  if (strcmp(key, "id") == 0) {
    if (sscanf(value, "%d", &params->id) <= 0) {
      return HTTP_UNPROCESSABLE_ENTITY;
    }
  } else if (strcmp(key, "mode") == 0) {
    int mode;

    if ((mode = config_enum_to_value(spi_leds_test_mode_enum, value)) < 0) {
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
int spi_leds_api_test_read_form_params(struct http_request *request, struct spi_leds_api_test_params *params)
{
  char *key, *value;
  int err;

  while (!(err = http_request_form(request, &key, &value))) {
    if ((err = spi_leds_api_test_params_set(params, key, value))) {
      LOG_WARN("spi_leds_api_test_params_set: %s=%s", key, value ? value : "");
      return err;
    }
  }

  if (err < 0) {
    LOG_ERROR("http_request_form");
    return err;
  }

  return 0;
}

int spi_leds_api_test_post(struct http_request *request, struct http_response *response, void *ctx)
{
  const struct http_request_headers *headers;
  struct spi_leds_api_test_params params = {
    .mode = TEST_MODE_CHASE,
  };
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  switch (headers->content_type) {
    case HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED:
      if ((err = spi_leds_api_test_read_form_params(request, &params))) {
        LOG_WARN("spi_leds_api_test_read_form_params");
        return err;
      }

      break;

    default:
      LOG_WARN("Unknown Content-Type");

      return HTTP_UNSUPPORTED_MEDIA_TYPE;
  }

  // decode
  struct spi_leds_state *state;

  if (params.id < 0 || params.id >= SPI_LEDS_COUNT) {
    LOG_WARN("invalid id=%d", params.id);
    return HTTP_UNPROCESSABLE_ENTITY;
  } else {
    state = &spi_leds_states[params.id];
  }

  // XXX: may block for some time
  if ((err = test_spi_leds(state, params.mode)) < 0) {
    LOG_ERROR("test_spi_leds");
    return HTTP_INTERNAL_SERVER_ERROR;
  } else if (err) {
    LOG_WARN("test_spi_leds");
    return HTTP_CONFLICT;
  }

  return HTTP_NO_CONTENT;
}
