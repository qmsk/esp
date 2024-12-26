#include "leds.h"
#include "leds_state.h"
#include "leds_artnet.h"
#include "leds_test.h"
#include "leds_config.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <leds_status.h>
#include <logging.h>
#include <json.h>

#include <limits.h>
#include <string.h>

static int leds_api_write_object_options(struct json_writer *w, struct leds_state *state)
{
  const struct leds_options *options = leds_options(state->leds);

  return (
        JSON_WRITE_MEMBER_STRING(w, "interface", config_enum_to_string(leds_interface_enum, options->interface))
    ||  JSON_WRITE_MEMBER_STRING(w, "protocol", config_enum_to_string(leds_protocol_enum, options->protocol))
    ||  JSON_WRITE_MEMBER_STRING(w, "parameter_type", config_enum_to_string(leds_parameter_enum, leds_parameter_type(state->leds)))
    ||  JSON_WRITE_MEMBER_UINT(w, "count", options->count)
    ||  JSON_WRITE_MEMBER_UINT(w, "limit_total", options->limit_total)
    ||  JSON_WRITE_MEMBER_UINT(w, "limit_group", options->limit_group)
    ||  JSON_WRITE_MEMBER_UINT(w, "limit_groups", options->limit_groups)
  );
}

static int leds_api_write_object_leds_limit_status(struct json_writer *w, const struct leds_limit_status *status)
{
  return (
        JSON_WRITE_MEMBER_UINT(w, "count", status->count)
    ||  JSON_WRITE_MEMBER_UINT(w, "limit", status->limit)
    ||  JSON_WRITE_MEMBER_UINT(w, "power", status->power)
    ||  JSON_WRITE_MEMBER_UINT(w, "output", status->output)
  );
}

static int leds_api_write_object_leds_limit_status_groups(struct json_writer *w, const struct leds_limit_status *groups_status, size_t groups)
{
  int err;

  for (unsigned i = 0; i < groups; i++) {
    if ((err = JSON_WRITE_OBJECT(w, leds_api_write_object_leds_limit_status(w, &groups_status[i])))) {
      return err;
    }
  }

  return 0;
}

static int leds_api_write_object_status(struct json_writer *w, struct leds_state *state)
{
  struct leds_limit_status limit_total_status;
  struct leds_limit_status limit_groups_status[LEDS_LIMIT_GROUPS_MAX];
  size_t groups = LEDS_LIMIT_GROUPS_MAX;
  bool active = leds_is_active(state->leds);
  TickType_t tick = xTaskGetTickCount();

  leds_get_limit_total_status(state->leds, &limit_total_status);
  leds_get_limit_groups_status(state->leds, limit_groups_status, &groups);

  return (
        JSON_WRITE_MEMBER_BOOL(w, "active", active)
    ||  JSON_WRITE_MEMBER_UINT(w, "update_tick", state->update_tick)
    ||  JSON_WRITE_MEMBER_UINT(w, "update_ms", state->update_tick ? (tick - state->update_tick) * portTICK_RATE_MS : 0)
    ||  JSON_WRITE_MEMBER_UINT(w, "artnet_dmx_ms", (state->artnet && state->artnet->dmx_tick) ? (tick - state->artnet->dmx_tick) * portTICK_RATE_MS : 0 )
    ||  JSON_WRITE_MEMBER_STRING(w, "test_mode", (state->test && state->test->mode) ? config_enum_to_string(leds_test_mode_enum, state->test->mode) : "")
    ||  JSON_WRITE_MEMBER_OBJECT(w, "limit_total", leds_api_write_object_leds_limit_status(w, &limit_total_status))
    ||  JSON_WRITE_MEMBER_ARRAY(w, "limit_groups", leds_api_write_object_leds_limit_status_groups(w, limit_groups_status, groups))
  );
}

static int leds_api_write_object_artnet(struct json_writer *w, struct leds_state *state)
{
  return (
        JSON_WRITE_MEMBER_UINT(w, "universe_start", state->config->artnet_universe_start)
    ||  JSON_WRITE_MEMBER_UINT(w, "universe_count", state->artnet->universe_count)
    ||  JSON_WRITE_MEMBER_UINT(w, "universe_leds", state->artnet->universe_leds_count)
    ||  JSON_WRITE_MEMBER_UINT(w, "leds_segment", state->config->artnet_leds_segment)
    ||  JSON_WRITE_MEMBER_UINT(w, "leds_group", state->config->artnet_leds_group)
    ||  JSON_WRITE_MEMBER_STRING(w, "leds_format", config_enum_to_string(leds_format_enum, state->config->artnet_leds_format))
    ||  JSON_WRITE_MEMBER_UINT(w, "dmx_timeout_ms", state->config->artnet_dmx_timeout)
  );
}

static int leds_api_write_object(struct json_writer *w, struct leds_state *state)
{
  return (
        JSON_WRITE_MEMBER_UINT(w, "index", state->index + 1)
    ||  JSON_WRITE_MEMBER_OBJECT(w, "options", leds_api_write_object_options(w, state))
    ||  JSON_WRITE_MEMBER_OBJECT(w, "status", leds_api_write_object_status(w, state))
    ||  (state->artnet ? JSON_WRITE_MEMBER_OBJECT(w, "artnet", leds_api_write_object_artnet(w, state)) : 0)
  );
}

static int leds_api_write_objects(struct json_writer *w)
{
  int err;

  for (int i = 0; i < LEDS_COUNT; i++)
  {
    struct leds_state *state = &leds_states[i];
    char name[10];

    if (!state->config->enabled || !state->leds) {
      continue;
    }

    snprintf(name, sizeof(name), "leds%u", state->index + 1);

    if ((err = JSON_WRITE_MEMBER_OBJECT(w, name, leds_api_write_object(w, state)))) {
      return err;
    }
  }

  return 0;
}

static int leds_api_write(struct json_writer *w, void *ctx)
{
  return JSON_WRITE_OBJECT(w, leds_api_write_objects(w));
}

int leds_api_get(struct http_request *request, struct http_response *response, void *ctx)
{
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = write_http_response_json(response, leds_api_write, NULL))) {
    LOG_WARN("write_http_response_json -> leds_api_write");
    return err;
  }

  return 0;
}

struct leds_api_query {
  struct leds_state *state;
};

int leds_api_query(struct http_request *request, struct leds_api_query *query)
{
  char *key, *value;
  int index;
  int err;

  while (!(err = http_request_query(request, &key, &value))) {
    if (strcmp(key, "leds") == 0) {
      if (sscanf(value, "leds%d", &index) <= 0) {
        LOG_WARN("invalid ?leds=%s", value);
        return HTTP_UNPROCESSABLE_ENTITY;
      } else if (index > 0 && index <= LEDS_COUNT) {
        query->state = &leds_states[index - 1];
      } else {
        LOG_WARN("invalid ?leds=%s", value);
        return HTTP_UNPROCESSABLE_ENTITY;
      }

      return 0;
    }
  }

  if (!query->state) {
    LOG_WARN("missing ?leds=...");
    return HTTP_UNPROCESSABLE_ENTITY;
  }


  return 0;
}

static int leds_api_write_status(struct json_writer *w, void *ctx)
{
  struct leds_api_query *query = ctx;

  return JSON_WRITE_OBJECT(w, leds_api_write_object_status(w, query->state));
}

int leds_api_get_status(struct http_request *request, struct http_response *response, void *ctx)
{
  struct leds_api_query query = {};
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = leds_api_query(request, &query))) {
    LOG_WARN("leds_api_query");
    return err;
  }

  if ((err = write_http_response_json(response, leds_api_write_status, &query))) {
    LOG_WARN("write_http_response_json -> leds_api_write_status");
    return err;
  }

  return 0;
}

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
