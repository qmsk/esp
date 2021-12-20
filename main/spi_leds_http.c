#include "spi_leds.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>
#include <json.h>

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

static int spi_leds_api_write_object(struct json_writer *w, int index, struct spi_leds_state *state)
{
  bool enabled = state->config->enabled && state->spi_leds;

  return (
        JSON_WRITE_MEMBER_UINT(w, "index", index)
    ||  JSON_WRITE_MEMBER_BOOL(w, "enabled", enabled)
    ||  (enabled ? spi_leds_api_write_object_enabled(w, index, state) : 0)
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

int spi_leds_api_handler(struct http_request *request, struct http_response *response, void *ctx)
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
