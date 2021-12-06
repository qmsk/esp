#include "artnet.h"
#include "http_routes.h"

#include <artnet.h>
#include <artnet_stats.h>

#include <logging.h>
#include <json.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

static int write_json_response(struct http_response *response, int(*f)(struct json_writer *w))
{
  struct json_writer json_writer;
  FILE *file;
  int err;

  if ((err = http_response_start(response, HTTP_OK, NULL))) {
    LOG_WARN("http_response_start");
    return err;
  }

  if ((err = http_response_header(response, "Content-Type", "application/json"))) {
    LOG_WARN("http_response_header");
    return err;
  }

  if ((err = http_response_open(response, &file))) {
    LOG_WARN("http_response_open");
    return err;
  }

  if (json_writer_init(&json_writer, file)) {
    LOG_ERROR("json_writer_init");
    return -1;
  }

  if ((err = f(&json_writer))) {
    return -1;
  }

  if (fclose(file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

  return 0;
}

static int artnet_api_write_input_object(struct json_writer *w, const struct artnet_input_options *options)
{
  return (
        JSON_WRITE_MEMBER_UINT(w, "port", options->port)
    ||  JSON_WRITE_MEMBER_UINT(w, "index", options->index)
    ||  JSON_WRITE_MEMBER_UINT(w, "net", artnet_address_net(options->address))
    ||  JSON_WRITE_MEMBER_UINT(w, "subnet", artnet_address_subnet(options->address))
  );
}

static int artnet_api_write_inputs_array(struct json_writer *w)
{
  struct artnet_input_options options;
  int err;

  for (int index = 0; !(err = artnet_get_input_options(artnet, index, &options)); index++) {
    if ((err = JSON_WRITE_OBJECT(w, artnet_api_write_input_object(w, &options)))) {
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
  return (
        JSON_WRITE_MEMBER_UINT(w, "port", options->port)
    ||  JSON_WRITE_MEMBER_UINT(w, "index", options->index)
    ||  JSON_WRITE_MEMBER_UINT(w, "net", artnet_address_net(options->address))
    ||  JSON_WRITE_MEMBER_UINT(w, "subnet", artnet_address_subnet(options->address))
    ||  (options->task ? JSON_WRITE_MEMBER_STRING(w, "task", pcTaskGetName(options->task)) : 0)
    ||  JSON_WRITE_MEMBER_OBJECT(w, "state",
            JSON_WRITE_MEMBER_UINT(w, "seq", state->seq)
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

static int artnet_api_write(struct json_writer *w)
{
  struct artnet_options options = artnet_get_options(artnet);

  return JSON_WRITE_OBJECT(w,
        JSON_WRITE_MEMBER_OBJECT(w, "config",
            JSON_WRITE_MEMBER_UINT(w, "port", options.port)
        ||  JSON_WRITE_MEMBER_UINT(w, "net", artnet_address_net(options.address))
        ||  JSON_WRITE_MEMBER_UINT(w, "subnet", artnet_address_subnet(options.address))
        )
    ||  JSON_WRITE_MEMBER_OBJECT(w, "status",
            JSON_WRITE_MEMBER_RAW(w, "ip_address", "\"%u.%u.%u.%u\"",
              options.ip_address[0],
              options.ip_address[1],
              options.ip_address[2],
              options.ip_address[3]
            )
        ||  JSON_WRITE_MEMBER_RAW(w, "mac_address", "\"%02x:%02x:%02x:%02x:%02x:%02x\"",
              options.mac_address[0],
              options.mac_address[1],
              options.mac_address[2],
              options.mac_address[3],
              options.mac_address[4],
              options.mac_address[5]
            )
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

  if ((err = write_json_response(response, artnet_api_write))) {
    LOG_WARN("write_json_response -> artnet_api_write");
    return err;
  }

  return 0;
}
