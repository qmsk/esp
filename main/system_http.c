#include "http_routes.h"

#include <logging.h>

#include <esp_system.h>

int system_api_restart_handler(struct http_request *request, struct http_response *response, void *ctx)
{
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = http_response_start(response, HTTP_NO_CONTENT, NULL))) {
    LOG_WARN("http_response_start");
    return err;
  }

  if ((err = http_response_close(response))) {
    LOG_WARN("http_response_close");
    return err;
  }

  LOG_INFO("restarting...");

  // this function does not return
  esp_restart();
}
