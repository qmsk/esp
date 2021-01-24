#include "config.h"
#include "http_routes.h"

#include <lib/logging.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

int config_get_handler(struct http_request *request, struct http_response *response, void *ctx)
{
  FILE *file;
  int err;

  LOG_INFO("return ");

  if ((err = http_response_start(response, HTTP_OK, NULL))) {
    LOG_WARN("http_response_start");
    return err;
  }

  if ((err = http_response_header(response, "Content-Type", "text/plain"))) {
    LOG_WARN("http_response_header");
    return err;
  }

  if ((err = http_response_open(response, &file))) {
    LOG_WARN("http_response_open");
    return err;
  }

  LOG_DEBUG("file=%p", file);

  if ((err = config_write(&config, file))) {
    LOG_WARN("config_write");
    return -1;
  }

  if (fclose(file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

  return 0;

}
