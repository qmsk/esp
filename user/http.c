#include "http.h"

#include <lib/httpserver/server.h>
#include <lib/logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#define HTTP_CONFIG_HOST "0.0.0.0"
#define HTTP_CONFIG_PORT 80

#define HTTP_LISTEN_BACKLOG 4
#define HTTP_STREAM_SIZE 1024 // 1+1kB per connection

#define HTTP_CONNECTION_QUEUE_SIZE 1
#define HTTP_CONNECTION_QUEUE_TIMEOUT 1000 // 1s

struct http_config {
  const char *host;
  int port;
} http_config = {
  .host = HTTP_CONFIG_HOST,
  .port = HTTP_CONFIG_PORT,
};

struct user_http {
  struct http_server *server;
  struct http_listener *listener;

  xTaskHandle listen_task, server_task;
  xQueueHandle connection_queue;
} http;

int http_handler(struct http_request *request, struct http_response *response, void *ctx)
{
  FILE *file;
  int err;

  LOG_INFO("write text/plain response");

  if ((err = http_response_start(response, HTTP_OK, NULL))) {
    LOG_WARN("http_response_start");
    return err;
  }

  if ((err = http_response_header(response, "Content-Type", "text/plain"))) {
    LOG_WARN("http_response_header");
    return err;
  }

  if ((err = http_response_open(response, &file))) {
    LOG_WARN("http_response_file");
    return err;
  }

  LOG_DEBUG("file=%p", file);

  if (fprintf(file, "Hello World!\n") < 0) {
    LOG_WARN("fprintf: %s", strerror(errno));
    return -1;
  }

  if (fclose(file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

  return 0;
}

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
  struct http_connection *connection;
  int err;

  for (;;) {
    if (!xQueueReceive(http.connection_queue, &connection, portMAX_DELAY)) {
      LOG_ERROR("xQueueReceive");
      break;
    }

    LOG_DEBUG("connection=%p serve", connection);

    if ((err = http_connection_serve(connection, &http_handler, NULL)) < 0) {
      LOG_ERROR("http_connection_serve");
      http_connection_destroy(connection);
      continue;
    } else if (err > 0) {
      LOG_INFO("force-close HTTP connection");
      http_connection_destroy(connection);
    } else {
      LOG_DEBUG("closing HTTP connection");
      http_connection_destroy(connection);
    }
  }

  vTaskDelete(NULL);
}

int init_http(struct http_config *config)
{
  char port[32];
  int err;

  if (snprintf(port, sizeof(port), "%d", config->port) >= sizeof(port)) {
    LOG_ERROR("snprintf port");
    return -1;
  }

  if ((err = http_server_create(&http.server, HTTP_LISTEN_BACKLOG, HTTP_STREAM_SIZE))) {
    LOG_ERROR("http_server_create");
    return err;
  }

  LOG_INFO("listen TCP %s:%s", config->host, port);

  if ((err = http_server_listen(http.server, config->host, port, &http.listener))) {
    LOG_ERROR("http_server_listen");
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

  if (xTaskCreate(&http_server_task, (signed char *) "http-server", HTTP_SERVER_TASK_SIZE, NULL, tskIDLE_PRIORITY + 2, &http.server_task) <= 0) {
    LOG_ERROR("xTaskCreate http-server");
    return -1;
  }

  return 0;
}
