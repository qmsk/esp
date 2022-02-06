#include "http_handlers.h"

#include <logging.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

int write_http_response_json(struct http_response *response, int(*f)(struct json_writer *w, void *ctx), void *ctx)
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

  if ((err = f(&json_writer, ctx))) {
    return -1;
  }

  if (fclose(file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

  return 0;
}
