#include "artnet.h"
#include "artnet_state.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <artnet.h>
#include <artnet_stats.h>

#include <logging.h>
#include <json.h>

static int artnet_api_write_input_object(struct json_writer *w, const struct artnet_input_options *options, const struct artnet_input_state *state)
{
  TickType_t tick = xTaskGetTickCount();

  return (
        JSON_WRITE_MEMBER_UINT(w, "port", options->port)
    ||  JSON_WRITE_MEMBER_UINT(w, "index", options->index)
    ||  JSON_WRITE_MEMBER_UINT(w, "net", artnet_address_net(options->address))
    ||  JSON_WRITE_MEMBER_UINT(w, "subnet", artnet_address_subnet(options->address))
    ||  JSON_WRITE_MEMBER_UINT(w, "universe", artnet_address_universe(options->address))
    ||  JSON_WRITE_MEMBER_OBJECT(w, "state",
              JSON_WRITE_MEMBER_UINT(w, "tick", state->tick)
          ||  JSON_WRITE_MEMBER_UINT(w, "tick_ms", state->tick ? (tick - state->tick) / portTICK_RATE_MS : 0)
          ||  JSON_WRITE_MEMBER_UINT(w, "len", state->len)
        )
  );
}

static int artnet_api_write_inputs_array(struct json_writer *w)
{
  struct artnet_input_options options;
  struct artnet_input_state state;
  int err;

  for (int index = 0; !(err = artnet_get_input_options(artnet, index, &options)); index++) {
    if ((err = artnet_get_input_state(artnet, index, &state))) {
      LOG_WARN("artnet_get_input_state");
      state = (struct artnet_input_state) {};
    }

    if ((err = JSON_WRITE_OBJECT(w, artnet_api_write_input_object(w, &options, &state)))) {
      return err;
    }
  }

  if (err < 0) {
    return err;
  }

  return 0;
}

static int artnet_api_write_output_object(struct json_writer *w, const struct artnet_output_options *options, const struct artnet_output_state *state)
{
  TickType_t tick = xTaskGetTickCount();

  return (
        JSON_WRITE_MEMBER_UINT(w, "port", options->port)
    ||  JSON_WRITE_MEMBER_UINT(w, "index", options->index)
    ||  JSON_WRITE_MEMBER_NSTRING(w, "name", options->name)
    ||  JSON_WRITE_MEMBER_UINT(w, "net", artnet_address_net(options->address))
    ||  JSON_WRITE_MEMBER_UINT(w, "subnet", artnet_address_subnet(options->address))
    ||  JSON_WRITE_MEMBER_UINT(w, "universe", artnet_address_universe(options->address))
    ||  JSON_WRITE_MEMBER_OBJECT(w, "state",
              JSON_WRITE_MEMBER_UINT(w, "tick", state->tick)
          ||  JSON_WRITE_MEMBER_UINT(w, "tick_ms", state->tick ? (tick - state->tick) / portTICK_RATE_MS : 0)
          ||  JSON_WRITE_MEMBER_UINT(w, "seq", state->seq)
        )
  );
}


static int artnet_api_write_outputs_array(struct json_writer *w)
{
  struct artnet_output_options options;
  struct artnet_output_state state;
  int err;

  for (int index = 0; !(err = artnet_get_output_options(artnet, index, &options)); index++) {
    if ((err = artnet_get_output_state(artnet, index, &state))) {
      LOG_WARN("artnet_get_output_state");
      state = (struct artnet_output_state) {};
    }

    if ((err = JSON_WRITE_OBJECT(w, artnet_api_write_output_object(w, &options, &state)))) {
      return err;
    }
  }

  if (err < 0) {
    return err;
  }

  return 0;
}

static int artnet_api_write(struct json_writer *w, void *ctx)
{
  struct artnet *artnet = ctx;
  struct artnet_options options = artnet_get_options(artnet);

  return JSON_WRITE_OBJECT(w,
        JSON_WRITE_MEMBER_OBJECT(w, "config",
            JSON_WRITE_MEMBER_UINT(w, "port", options.port)
        ||  JSON_WRITE_MEMBER_UINT(w, "net", artnet_address_net(options.address))
        ||  JSON_WRITE_MEMBER_UINT(w, "subnet", artnet_address_subnet(options.address))
        )
    ||  JSON_WRITE_MEMBER_OBJECT(w, "metadata",
            JSON_WRITE_MEMBER_RAW(w, "ip_address", "\"%u.%u.%u.%u\"",
              options.metadata.ip_address[0],
              options.metadata.ip_address[1],
              options.metadata.ip_address[2],
              options.metadata.ip_address[3]
            )
        ||  JSON_WRITE_MEMBER_RAW(w, "mac_address", "\"%02x:%02x:%02x:%02x:%02x:%02x\"",
              options.metadata.mac_address[0],
              options.metadata.mac_address[1],
              options.metadata.mac_address[2],
              options.metadata.mac_address[3],
              options.metadata.mac_address[4],
              options.metadata.mac_address[5]
            )
        ||  JSON_WRITE_MEMBER_STRING(w, "short_name", options.metadata.short_name)
        ||  JSON_WRITE_MEMBER_STRING(w, "long_name", options.metadata.long_name)
        )
    ||  JSON_WRITE_MEMBER_ARRAY(w, "inputs", artnet_api_write_inputs_array(w))
    ||  JSON_WRITE_MEMBER_ARRAY(w, "outputs", artnet_api_write_outputs_array(w))
  );
}

int artnet_api_handler(struct http_request *request, struct http_response *response, void *ctx)
{
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if (!artnet) {
    // disabled
    return HTTP_NO_CONTENT;
  }

  if ((err = write_http_response_json(response, artnet_api_write, artnet))) {
    LOG_WARN("write_http_response_json -> artnet_api_write");
    return err;
  }

  return 0;
}
