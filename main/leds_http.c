#include "leds.h"
#include "leds_api.h"
#include "leds_state.h"
#include "leds_status.h"
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

#define TICK_MS(current_tick, tick) (tick ? (current_tick - tick) * portTICK_RATE_MS : 0)

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

static int leds_api_write_object_leds_limit_status_groups(struct json_writer *w, const struct leds_limit_status *groups_status, size_t count)
{
  int err;

  for (unsigned i = 0; i < count; i++) {
    if ((err = JSON_WRITE_OBJECT(w, leds_api_write_object_leds_limit_status(w, &groups_status[i])))) {
      return err;
    }
  }

  return 0;
}

static int leds_api_write_object_status_timer_metrics(struct json_writer *w, const struct stats_timer_metrics *metrics)
{
  return (
        JSON_WRITE_MEMBER_FLOAT(w, "interval", metrics->interval)
    ||  JSON_WRITE_MEMBER_FLOAT(w, "rate", metrics->rate)
    ||  JSON_WRITE_MEMBER_FLOAT(w, "util", metrics->util)
  );
}

static int leds_api_write_object_status(struct json_writer *w, struct leds_state *state)
{
  struct leds_status status;

  get_leds_status(state, &status);

  return (
        JSON_WRITE_MEMBER_BOOL(w, "active", status.active)
    ||  JSON_WRITE_MEMBER_UINT(w, "update_tick", status.update_tick)
    ||  JSON_WRITE_MEMBER_UINT(w, "update_ms", TICK_MS(status.tick, status.update_tick))
    ||  JSON_WRITE_MEMBER_UINT(w, "artnet_dmx_ms", status.artnet ? TICK_MS(status.tick, status.artnet_dmx_tick) : 0)
    ||  JSON_WRITE_MEMBER_OBJECT(w, "metrics",
              JSON_WRITE_MEMBER_OBJECT(w, "task", leds_api_write_object_status_timer_metrics(w, &status.metrics.task))
          ||  JSON_WRITE_MEMBER_OBJECT(w, "interface", leds_api_write_object_status_timer_metrics(w, &status.metrics.interface))
        )
    ||  JSON_WRITE_MEMBER_STRING(w, "test_mode", (status.test && status.test_mode) ? config_enum_to_string(leds_test_mode_enum, status.test_mode) : "")
    ||  JSON_WRITE_MEMBER_OBJECT(w, "limit_total", leds_api_write_object_leds_limit_status(w, &status.limit_total_status))
    ||  JSON_WRITE_MEMBER_ARRAY(w, "limit_groups", leds_api_write_object_leds_limit_status_groups(w, status.limit_groups_status, status.limit_groups_count))
  );
}

static int leds_api_write_object_artnet(struct json_writer *w, struct leds_state *state)
{
  return (
        JSON_WRITE_MEMBER_UINT(w, "net", state->config->artnet_net)
    ||  JSON_WRITE_MEMBER_UINT(w, "subnet", state->config->artnet_subnet)
    ||  JSON_WRITE_MEMBER_UINT(w, "universe_start", state->config->artnet_universe_start)
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
    ||  JSON_WRITE_MEMBER_OBJECT(w, "static",
          JSON_WRITE_MEMBER(w, "color", leds_api_write_color(w, state->static_.color, leds_parameter_type_for_protocol(leds_protocol(state->leds))))
        )
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
  int err;

  while (!(err = http_request_query(request, &key, &value))) {
    if (strcmp(key, "leds") == 0) {
      return leds_api_leds_parse(&query->state, value);
    }
  }

  if (err < 0) {
    LOG_WARN("http_request_query");
    return err;
  }

  if (!query->state) {
    LOG_WARN("missing leds=...");
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
