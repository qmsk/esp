#include "http.h"
#include "http_routes.h"

#include <esp_ota_ops.h>
#include <httpserver/server.h>
#include <httpserver/router.h>
#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <string.h>

#define HTTP_LISTEN_BACKLOG 4
#define HTTP_STREAM_SIZE 1024 // 1+1kB per connection

#define HTTP_CONNECTION_QUEUE_SIZE 1
#define HTTP_CONNECTION_QUEUE_TIMEOUT 1000 // 1s

#define HTTP_LISTEN_TASK_STACK 1024
#define HTTP_LISTEN_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define HTTP_SERVER_TASK_STACK 2048
#define HTTP_SERVER_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

#define HTTP_CONFIG_ENABLED true
#define HTTP_CONFIG_HOST "0.0.0.0"
#define HTTP_CONFIG_PORT 80

struct http_config {
  bool     enabled;
  char     host[32];
  uint16_t port;
} http_config = {
  .enabled  = HTTP_CONFIG_ENABLED,
  .host     = HTTP_CONFIG_HOST,
  .port     = HTTP_CONFIG_PORT,
};

const struct configtab http_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .value  = { .boolean = &http_config.enabled },
  },
  { CONFIG_TYPE_STRING, "host",
    .size   = sizeof(http_config.host),
    .value  = { .string = http_config.host },
  },
  { CONFIG_TYPE_UINT16, "port",
    .value  = { .uint16 = &http_config.port },
  },
  {}
};

static struct http_main {
  struct http_server *server;
  struct http_listener *listener;
  struct http_router *router;

  xTaskHandle listen_task, server_task;
  xQueueHandle connection_queue;
} http;

void http_listen_task(void *arg)
{
  struct http_listener *listener = arg;
  struct http_connection *connection;
  int err;

  for (;;) {
    LOG_DEBUG("listener=%p accept", listener);

    if ((err = http_listener_accept(listener, &connection))) {
      LOG_WARN("http_listener_accept");
      continue;
    }

    LOG_DEBUG("listener=%p accepted connection=%p", listener, connection);

    if ((err = xQueueSend(http.connection_queue, &connection, HTTP_CONNECTION_QUEUE_TIMEOUT / portTICK_RATE_MS)) == pdTRUE) {

    } else if (err == errQUEUE_FULL) {
      LOG_WARN("connection queue overflow, rejecting connection");
      http_connection_destroy(connection);
      continue;
    } else {
      LOG_ERROR("xQueueSend");
      break;
    }
  }

  vTaskDelete(NULL);
}

/* Hook to add response headers */
static int http_server_response_headers(struct http_response *response, void *ctx)
{
  const esp_app_desc_t *ead = esp_ota_get_app_description();
  int err;

  if ((err = http_response_header(response, "Server", "%s/%s (%s %s) ESP-IDF/%s", ead->project_name, ead->version, ead->date, ead->time, ead->idf_ver))) {
    LOG_ERROR("http_response_header");
    return err;
  }

  return 0;
}

void http_server_task(void *arg)
{
  struct http_router *router = arg;
  struct http_hook http_hook_response_headers = {
    .func.response_headers = http_server_response_headers,
  };
  struct http_hooks hooks = {
    .types = {
      [HTTP_HOOK_RESPONSE] = &http_hook_response_headers,
    },
  };
  struct http_connection *connection;
  int err;

  for (;;) {
    if (!xQueueReceive(http.connection_queue, &connection, portMAX_DELAY)) {
      LOG_ERROR("xQueueReceive");
      break;
    }

    LOG_DEBUG("connection=%p serve router=%p", connection, router);

    if ((err = http_connection_serve(connection, &hooks, &http_router_handler, router)) < 0) {
      LOG_ERROR("http_connection_serve");
      http_connection_destroy(connection);
      continue;
    } else if (err > 0) {
      LOG_DEBUG("closing HTTP connection");
      http_connection_destroy(connection);
    } else {
      LOG_INFO("force-close HTTP connection");
      http_connection_destroy(connection);
    }
  }

  vTaskDelete(NULL);
}

int setup_http(struct http_main *http, struct http_config *config)
{
  char port[32];
  int err;

  // router routes
  for (const struct http_route *route = http_routes; route->method || route->path || route->handler; route++) {
    LOG_INFO("route %s:%s", route->method, route->path);

    if ((err = http_router_add(http->router, route))) {
      LOG_ERROR("http_router_add");
      return err;
    }
  }

  // server listeners
  if (snprintf(port, sizeof(port), "%d", config->port) >= sizeof(port)) {
    LOG_ERROR("snprintf port");
    return -1;
  }

  LOG_INFO("listen TCP %s:%s", config->host, port);

  if ((err = http_server_listen(http->server, config->host, port, &http->listener))) {
    LOG_ERROR("http_server_listen");
    return err;
  }

  return 0;
}

int init_http()
{
  int err;

  if (!http_config.enabled) {
    LOG_INFO("disabled");
    return 1;
  }

  if ((err = http_server_create(&http.server, HTTP_LISTEN_BACKLOG, HTTP_STREAM_SIZE))) {
    LOG_ERROR("http_server_create");
    return err;
  }

  if ((err = http_router_create(&http.router))) {
    LOG_ERROR("http_router_create");
    return err;
  }

  if ((err = setup_http(&http, &http_config))) {
    LOG_ERROR("setup_http");
    return err;
  }

  if ((err = init_http_dist(&http, &http_config))) {
    LOG_ERROR("init_http_dist");
    return err;
  }

  if ((http.connection_queue = xQueueCreate(HTTP_CONNECTION_QUEUE_SIZE, sizeof(struct http_connection *))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  if (xTaskCreate(&http_listen_task, "http-listen", HTTP_LISTEN_TASK_STACK, http.listener, HTTP_LISTEN_TASK_PRIORITY, &http.listen_task) <= 0) {
    LOG_ERROR("xTaskCreate http-listen");
    return -1;
  }

  if (xTaskCreate(&http_server_task, "http-server", HTTP_SERVER_TASK_STACK, http.router, HTTP_SERVER_TASK_PRIORITY, &http.server_task) <= 0) {
    LOG_ERROR("xTaskCreate http-server");
    return -1;
  }

  return 0;
}
