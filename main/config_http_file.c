#include "config.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_HTTP_FILENAME "config.ini"
#define CONFIG_HTTP_CONTENT_TYPE "text/plain"

int config_file_get(struct http_request *request, struct http_response *response, void *ctx)
{
  FILE *file;
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  LOG_INFO("return ");

  if ((err = http_response_start(response, HTTP_OK, NULL))) {
    LOG_WARN("http_response_start");
    return err;
  }

  if ((err = http_response_header(response, "Content-Type", "%s", CONFIG_HTTP_CONTENT_TYPE))) {
    LOG_WARN("http_response_header");
    return err;
  }

  if ((err = http_response_header(response, "Content-Disposition", "attachment; filename=\"%s\"", CONFIG_HTTP_FILENAME))) {
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
    goto error;
  }

error:
  if (fclose(file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

  return err;
}

int config_file_post_ini(struct http_request *request, struct http_response *response, void *ctx)
{
  FILE *file;
  int err;

  if ((err = http_request_open(request, &file))) {
    LOG_WARN("http_request_open");
    return err;
  }

  LOG_INFO("config read...");
  LOG_DEBUG("file=%p", file);

  if ((err = config_init(&config))) {
    LOG_ERROR("config_init");
    err = HTTP_INTERNAL_SERVER_ERROR;
    goto error;
  }

  if ((err = config_read(&config, file))) {
    LOG_WARN("config_read");
    err = HTTP_UNPROCESSABLE_ENTITY;
    goto error;
  }

  LOG_INFO("config save %s", CONFIG_BOOT_FILE);

  if ((err = config_save(&config, CONFIG_BOOT_FILE))) {
    LOG_ERROR("config_save");
    goto error;
  }

  err = HTTP_NO_CONTENT;

error:
  if (fclose(file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

  return err;
}

int config_file_post(struct http_request *request, struct http_response *response, void *ctx)
{
  const struct http_request_headers *headers;
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  switch (headers->content_type) {
    case HTTP_CONTENT_TYPE_TEXT_PLAIN:
      return config_file_post_ini(request, response, ctx);

    default:
      LOG_WARN("Unkonwn Content-Type");

      return HTTP_UNSUPPORTED_MEDIA_TYPE;
  }
}
