#define DEBUG

#include "http.h"
#include "http_config.h"
#include "http_routes.h"

#include <lib/httpserver/server.h>
#include <lib/httpserver/router.h>
#include <lib/logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define HTTP_LISTEN_BACKLOG 4
#define HTTP_STREAM_SIZE 1024 // 1+1kB per connection

#define HTTP_CONNECTION_QUEUE_SIZE 1
#define HTTP_CONNECTION_QUEUE_TIMEOUT 1000 // 1s

struct http_config http_config = {
  .host = HTTP_CONFIG_HOST,
  .port = HTTP_CONFIG_PORT,
};

const struct configtab http_configtab[] = {
  { CONFIG_TYPE_STRING, "host",
    .size   = sizeof(http_config.host),
    .value  = { .string = http_config.host },
  },
  { CONFIG_TYPE_UINT16, "port",
    .value  = { .uint16 = &http_config.port },
  },
  {}
};

struct user_http {
  struct http_server *server;
  struct http_listener *listener;
  struct http_router *router;

  xTaskHandle listen_task, server_task;
  xQueueHandle connection_queue;
} http;

#define HTTP_LISTEN_TASK_SIZE 512
#define HTTP_SERVER_TASK_SIZE 1024

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

void http_server_task(void *arg)
{
  struct http_router *router = arg;
  struct http_connection *connection;
  int err;

  for (;;) {
    if (!xQueueReceive(http.connection_queue, &connection, portMAX_DELAY)) {
      LOG_ERROR("xQueueReceive");
      break;
    }

    LOG_DEBUG("connection=%p serve router=%p", connection, router);

    if ((err = http_connection_serve(connection, &http_router_handler, router)) < 0) {
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

int setup_http(struct user_http *http, struct http_config *config)
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

int init_http(struct http_config *config)
{
  int err;

  if (strlen(config->host) == 0 || config->port == 0) {
    LOG_INFO("disabled via config [http] host/port");
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

  if ((err = setup_http(&http, config))) {
    return err;
  }

  if ((http.connection_queue = xQueueCreate(HTTP_CONNECTION_QUEUE_SIZE, sizeof(struct http_connection *))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  if (xTaskCreate(&http_listen_task, (signed char *) "http-listen", HTTP_LISTEN_TASK_SIZE, http.listener, tskIDLE_PRIORITY + 2, &http.listen_task) <= 0) {
    LOG_ERROR("xTaskCreate http-listen");
    return -1;
  }

  if (xTaskCreate(&http_server_task, (signed char *) "http-server", HTTP_SERVER_TASK_SIZE, http.router, tskIDLE_PRIORITY + 2, &http.server_task) <= 0) {
    LOG_ERROR("xTaskCreate http-server");
    return -1;
  }

  return 0;
}
