#include "http.h"
#include "http_routes.h"
#include "tasks.h"

#include <esp_ota_ops.h>
#include <httpserver/server.h>
#include <httpserver/router.h>
#include <httpserver/auth.h>
#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <string.h>

#include <sdkconfig.h>

// maximum number of connections for listen() queue, pending for accept()
#define HTTP_LISTEN_BACKLOG 4

#define HTTP_STREAM_SIZE 1024 // 1+1kB per connection

// maximum number of supported HTTP connections at a time
#define HTTP_CONNECTION_COUNT 2

// number of accepted connections to queue up for serving
#define HTTP_CONNECTION_QUEUE_SIZE 2
#define HTTP_CONNECTION_QUEUE_TIMEOUT 1000 // 1s

#define HTTP_AUTHENTICATION_REALM "HTTP username/password"
#define HTTP_AUTHORIZATION_HEADER_MAX 64

#define HTTP_CONFIG_ENABLED true
#define HTTP_CONFIG_HOST "0.0.0.0"
#define HTTP_CONFIG_PORT 80

struct http_config {
  bool     enabled;
  char     host[32];
  uint16_t port;
  char     username[32];
  char     password[32];
} http_config = {};

const struct configtab http_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &http_config.enabled, .default_value = HTTP_CONFIG_ENABLED },
  },
  { CONFIG_TYPE_STRING, "host",
    .string_type = { .value = http_config.host, .size = sizeof(http_config.host), .default_value = HTTP_CONFIG_HOST },
  },
  { CONFIG_TYPE_UINT16, "port",
    .uint16_type = { .value = &http_config.port, .default_value = HTTP_CONFIG_PORT },
  },
  { CONFIG_TYPE_STRING, "username",
    .description = "Optional HTTP basic authentication username/password",
    .string_type = { .value = http_config.username, .size = sizeof(http_config.username) },
  },
  { CONFIG_TYPE_STRING, "password",
    .description = "Optional HTTP basic authentication username/password",
    .string_type = { .value = http_config.password, .size = sizeof(http_config.password) },
    .secret = true,
  },
  {}
};

static struct http_state {
  struct http_server *server;
  struct http_listener *listener;
  struct http_router *router;

  xTaskHandle listen_task, server_task;
  xQueueHandle accept_queue; // struct http_connection* for accept()
  xQueueHandle serve_queue; // struct http_connection* for serve()
} http_state;

int config_http(struct http_state *http, struct http_config *config)
{
  int err;

  if ((err = http_server_create(&http->server, HTTP_LISTEN_BACKLOG, HTTP_STREAM_SIZE))) {
    LOG_ERROR("http_server_create");
    return err;
  }

  if ((err = http_router_create(&http->router))) {
    LOG_ERROR("http_router_create");
    return err;
  }

  // router routes
  for (const struct http_route *route = http_routes; route->method || route->path || route->handler; route++) {
    LOG_INFO("route %s:%s", route->method, route->path);

    if ((err = http_router_add(http->router, route))) {
      LOG_ERROR("http_router_add");
      return err;
    }
  }

  // connection queues
  if ((http->accept_queue = xQueueCreate(HTTP_CONNECTION_COUNT, sizeof(struct http_connection *))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  if ((http->serve_queue = xQueueCreate(HTTP_CONNECTION_QUEUE_SIZE, sizeof(struct http_connection *))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  // pre-allocate connections
  for (int i = 0; i < HTTP_CONNECTION_COUNT; i++) {
    struct http_connection *connection;

    if ((err = http_connection_new(http->server, &connection))) {
      LOG_ERROR("http_connection_new");
      return err;
    }

    LOG_DEBUG("queue connection=%p", connection);

    if ((err = xQueueSend(http->accept_queue, &connection, 0)) == pdTRUE) {

    } else if (err == errQUEUE_FULL) {
      // should never happen, assuming pre-allocated connections
      LOG_WARN("accept queue overflow");
      http_connection_destroy(connection);
      continue;
    } else {
      LOG_ERROR("xQueueSend");
      return -1;
    }
  }

  return 0;
}

int init_http()
{
  int err;

  if (!http_config.enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  if ((err = config_http(&http_state, &http_config))) {
    LOG_ERROR("config_http");
    return err;
  }

  if ((err = init_http_dist())) {
    LOG_ERROR("init_http_dist");
    return err;
  }

  return 0;
}

void http_listen_main(void *arg)
{
  struct http_state *http = arg;
  struct http_listener *listener = http->listener;
  struct http_connection *connection;
  int err;

  for (;;) {
    LOG_DEBUG("listener=%p wait connection", listener);

    if (!xQueueReceive(http->accept_queue, &connection, portMAX_DELAY)) {
      LOG_ERROR("xQueueReceive");
      break;
    }

    LOG_DEBUG("listener=%p accept connection=%p", listener, connection);

    if ((err = http_listener_accept(listener, connection))) {
      LOG_WARN("http_listener_accept");
      continue;
    }

    LOG_DEBUG("listener=%p accepted connection=%p", listener, connection);

    if ((err = xQueueSend(http->serve_queue, &connection, HTTP_CONNECTION_QUEUE_TIMEOUT / portTICK_RATE_MS)) == pdTRUE) {
      LOG_DEBUG("listener=%p queued connection=%p", listener, connection);
    } else if (err == errQUEUE_FULL) {
      LOG_WARN("serve queue overflow, rejecting connection");
      http_connection_close(connection);
      continue;
    } else {
      LOG_ERROR("xQueueSend");
      break;
    }
  }

  // abort, reject further connections
  http_listener_destroy(listener);

  vTaskDelete(NULL);
}

struct http_context {
  const struct http_config *config;

  bool authentication_required;
  bool authenticated;
};

/* Hook to check authentication headers */
static int http_server_request_header(struct http_request *request, const char *header, const char *value, void *ctx)
{
  struct http_context *context = ctx;
  char buf[HTTP_AUTHORIZATION_HEADER_MAX];
  int err;

  if (context->authentication_required && strcasecmp(header, "Authorization") == 0) {
    const char *username, *password;

    if ((err = http_basic_authorization(value, buf, sizeof(buf), &username, &password))) {
      LOG_WARN("http_basic_authorization");
      return err;
    }

    if (strcmp(username, context->config->username) != 0 || strcmp(password, context->config->password) != 0) {
      LOG_WARN("failed authentication as username=%s with password=%s", username, password);

      context->authenticated = false;

      return 0;
    }

    LOG_INFO("authenticated as username=%s with password", username);

    context->authenticated = true;
  }

  return 0;
}

static int http_server_request_response(struct http_request *request, struct http_response *response, void *ctx)
{
  struct http_context *context = ctx;
  int err;

  if (context->authentication_required && !context->authenticated) {
    LOG_WARN("unauthenticated request, returning HTTP 401");

    if ((err = http_response_start(response, HTTP_UNAUTHORIZED, "Unauthorized"))) {
      LOG_ERROR("http_response_start");
      return err;
    }

    if ((err = http_response_header(response, "WWW-Authenticate", "Basic realm=\"%s\"", HTTP_AUTHENTICATION_REALM))) {
      LOG_ERROR("http_response_header");
      return err;
    }

    return 401;
  }

  return 0;
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

void http_server_main(void *arg)
{
  struct http_state *http = arg;
  struct http_router *router = http->router;
  struct http_context ctx;
  struct http_hook http_hook_request_header = {
    .func.request_header = http_server_request_header,
    .ctx = &ctx,
  };
  struct http_hook http_hook_request_response = {
    .func.request_response = http_server_request_response,
    .ctx = &ctx,
  };
  struct http_hook http_hook_response_headers = {
    .func.response_headers = http_server_response_headers,
    .ctx = &ctx,
  };
  struct http_hooks hooks = {
    .types = {
      [HTTP_HOOK_REQUEST_HEADER]    = &http_hook_request_header,
      [HTTP_HOOK_REQUEST_RESPONSE]  = &http_hook_request_response,
      [HTTP_HOOK_RESPONSE]          = &http_hook_response_headers,
    },
  };
  struct http_connection *connection;
  int err;

  for (;;) {
    LOG_DEBUG("wait connection");

    if (!xQueueReceive(http->serve_queue, &connection, portMAX_DELAY)) {
      LOG_ERROR("xQueueReceive");
      break;
    }

    LOG_DEBUG("serve connection=%p using router=%p", connection, router);

    ctx = (struct http_context) {
      .config = &http_config,

      .authentication_required = http_config.username[0] ? true : false,
      .authenticated = false,
    };

    if ((err = http_connection_serve(connection, &hooks, &http_router_handler, router)) < 0) {
      LOG_ERROR("http_connection_serve");
    } else if (err > 0) {
      // HTTP connection-close
    } else {
      LOG_INFO("force-close HTTP connection");
    }

    LOG_DEBUG("close connection=%p", connection);

    if ((err = http_connection_close(connection))) {
      LOG_WARN("http_connection_close");
    }

    LOG_DEBUG("return connection=%p", connection);

    if ((err = xQueueSend(http->accept_queue, &connection, 0)) == pdTRUE) {
      LOG_DEBUG("returned connection=%p", connection);
    } else if (err == errQUEUE_FULL) {
      // should never happen, assuming pre-allocated connections
      LOG_WARN("accept queue overflow");
      http_connection_destroy(connection);
      continue;
    } else {
      LOG_ERROR("xQueueSend");
      break;
    }
  }

  // abort, futher connections will stall and timeout
  vTaskDelete(NULL);
}

int start_http_listen(struct http_state *http, struct http_config *config)
{
  char port[32];
  int err;

  if (!http->server) {
    LOG_ERROR("http->server not initialized");
    return -1;
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

int start_http_tasks(struct http_state *http)
{
  struct task_options http_listen_options = {
    .main       = http_listen_main,
    .name       = HTTP_LISTEN_TASK_NAME,
    .stack_size = HTTP_LISTEN_TASK_STACK,
    .arg        = http,
    .priority   = HTTP_LISTEN_TASK_PRIORITY,
    .handle     = &http->listen_task,
    .affinity   = HTTP_LISTEN_TASK_AFFINITY,
  };

  if (start_task(http_listen_options)) {
    LOG_ERROR("start_task http-listen");
    return -1;
  }

  struct task_options http_server_options = {
    .main       = http_server_main,
    .name       = HTTP_SERVER_TASK_NAME,
    .stack_size = HTTP_SERVER_TASK_STACK,
    .arg        = http,
    .priority   = HTTP_SERVER_TASK_PRIORITY,
    .handle     = &http->server_task,
    .affinity   = HTTP_SERVER_TASK_AFFINITY,
  };

  if (start_task(http_server_options)) {
    LOG_ERROR("start_task http-server");
    return -1;
  }

  return 0;
}

int start_http()
{
  int err;

  if (!http_config.enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  if ((err = start_http_listen(&http_state, &http_config))) {
    LOG_ERROR("start_http_listen");
    return err;
  }

  if ((err = start_http_tasks(&http_state))) {
    LOG_ERROR("start_http_tasks");
    return err;
  }

  return 0;
}
