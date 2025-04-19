#include "config.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>
#include <json.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <freertos/task.h>

#define TICK_MS(current_tick, tick) (tick ? (current_tick - tick) * portTICK_RATE_MS : 0)

struct config_api_set {
  struct config *config;
  struct http_response *response;
  struct json_writer json_writer;
  FILE *file;

  bool set_errors;
  bool validation_errors;
};

static int config_api_start_response (struct config_api_set *ctx, enum http_status status, const char *reason)
{
  int err;

  if ((err = http_response_start(ctx->response, status, reason))) {
    LOG_WARN("http_response_start");
    return err;
  }

  if ((err = http_response_header(ctx->response, "Content-Type", "application/json"))) {
    LOG_WARN("http_response_header");
    return err;
  }

  if ((err = http_response_open(ctx->response, &ctx->file))) {
    LOG_WARN("http_response_open");
    return err;
  }

  if (json_writer_init(&ctx->json_writer, ctx->file)) {
    LOG_ERROR("json_writer_init");
    return -1;
  }

  if ((err = json_open_object(&ctx->json_writer))) {
    LOG_ERROR("json_open_object");
    return err;
  }

  return 0;
}

static int config_api_close_response (struct config_api_set *ctx)
{
  int err;

  if ((err = json_close_object(&ctx->json_writer))) {
    LOG_ERROR("json_close_object");
    return err;
  }

  if (ctx->file) {
    if (fclose(ctx->file) < 0) {
      LOG_WARN("fclose: %s", strerror(errno));
      return -1;
    } else {
      ctx->file = NULL;
    }
  }

  return 0;
}

static int config_api_set_error (struct config_api_set *ctx, const char *key, const char *module, const char *name, const char *value, const char *error)
{
  int err;

  if (!http_response_get_status(ctx->response)) {
    if ((err = config_api_start_response(ctx, HTTP_UNPROCESSABLE_ENTITY, NULL))) {
      return err;
    }
  }

  if (!ctx->set_errors) {
    if ((err = json_open_object_member(&ctx->json_writer, "set_errors"))) {
      return err;
    }
    if ((err = json_open_array(&ctx->json_writer))) {
      return err;
    }

    ctx->set_errors = true;
  }

  if (key) {
    LOG_WARN("%s=%s: %s", key, value, error);
  } else if (module && name) {
    LOG_WARN("[%s]%s=%s: %s", module, name, value, error);
  }

  return JSON_WRITE_OBJECT(&ctx->json_writer, 
        (key ? JSON_WRITE_MEMBER_STRING(&ctx->json_writer, "key", key) : 0)
    ||  (module ? JSON_WRITE_MEMBER_STRING(&ctx->json_writer, "module", module) : 0)
    ||  (name ? JSON_WRITE_MEMBER_STRING(&ctx->json_writer, "name", name) : 0)
    ||  (value ? JSON_WRITE_MEMBER_STRING(&ctx->json_writer, "value", value) : 0)
    ||  JSON_WRITE_MEMBER_STRING(&ctx->json_writer, "error", error)
  );
}

static int config_api_set_done (struct config_api_set *ctx)
{
  int err;

  if (!ctx->set_errors) {
    return 0;
  }

  if ((err = json_close_array(&ctx->json_writer))) {
    return err;
  }

  if ((err = config_api_close_response(ctx))) {
    return err;
  }

  return HTTP_UNPROCESSABLE_ENTITY;
}

static int config_api_validation_error (struct config_api_set *ctx, const struct config_path path, const char *fmt, va_list args)
{
  char buf[128];
  int ret, err;

  if (!http_response_get_status(ctx->response)) {
    if ((err = config_api_start_response(ctx, HTTP_UNPROCESSABLE_ENTITY, NULL))) {
      return err;
    }
  }

  if (!ctx->validation_errors) {
    if ((err = json_open_object_member(&ctx->json_writer, "validation_errors"))) {
      return err;
    }
    if ((err = json_open_array(&ctx->json_writer))) {
      return err;
    }

    ctx->validation_errors = true;
  }

  ret = vsnprintf(buf, sizeof(buf), fmt, args);

  if (ret >= sizeof(buf)) {
    // truncated
    buf[sizeof(buf) - 1] = '.';
    buf[sizeof(buf) - 2] = '.';
    buf[sizeof(buf) - 3] = '.';
  }

  LOG_WARN("[%s%d] %s: %s", path.mod->name, path.index, path.tab->name, buf);

  return JSON_WRITE_OBJECT(&ctx->json_writer, 
        JSON_WRITE_MEMBER_STRING(&ctx->json_writer, "module", path.mod->name)
    ||  JSON_WRITE_MEMBER_UINT(&ctx->json_writer, "index", path.index)
    ||  JSON_WRITE_MEMBER_STRING(&ctx->json_writer, "name", path.tab->name)
    ||  JSON_WRITE_MEMBER_STRING(&ctx->json_writer, "error", buf)
  );
}

static int config_api_validation_done (struct config_api_set *ctx)
{
  int err;

  if (!ctx->validation_errors) {
    return 0;
  }

  if ((err = json_close_array(&ctx->json_writer))) {
    return err;
  }

  if ((err = config_api_close_response(ctx))) {
    return err;
  }

  return HTTP_UNPROCESSABLE_ENTITY;
}

static int config_api_parse_name (char *key, const char **modulep, const char **namep)
{
  *modulep = *namep = NULL;

  for (char *c = key; *c; c++) {
    if (*c == '[') {
      *modulep = c+1;
    } else if (*c == ']' ) {
      *c = '\0';
      *namep = c+1;
    }
  }

  if (!*modulep) {
    return -1;
  }

  if (!*namep) {
    return -1;
  }

  return 0;
}

static int config_api_set (struct config_api_set *ctx, char *key, char *value)
{
  const char *module, *name;
  struct config_path path;
  int err;

  if ((err = config_api_parse_name(key, &module, &name))) {
    return config_api_set_error(ctx, key, NULL, NULL, value, "Invalid key");
  }

  if ((err = config_lookup(ctx->config, module, name, &path))) {
    return config_api_set_error(ctx, NULL, module, name, value, "Unknown config");
  }

  if (value && *value) {
    LOG_INFO("module=%s index=%u name=%s: set value=%s", path.mod->name, path.index, path.tab->name, value);

    if ((err = config_set(path, value))) {
      return config_api_set_error(ctx, NULL, module, name, value, "Invalid value");
    }
  } else {
    LOG_INFO("module=%s index=%u name=%s: clear", path.mod->name, path.index, path.tab->name);

    if ((err = config_clear(path))) {
      return config_api_set_error(ctx, NULL, module, name, value, "Invalid clear");
    }
  }

  return 0;
}

static void config_api_invalid(const struct config_path path, void *_ctx, const char *fmt, ...)
{
  struct config_api_set *ctx = _ctx;
  va_list args;
  int err;

  va_start(args, fmt);
  err = config_api_validation_error(ctx, path, fmt, args);
  va_end(args);

  if (err) {
    LOG_ERROR("config_api_validation_error");
  }
}

static int config_api_valid (struct config_api_set *ctx)
{
  int err;

  if ((err = config_valid(ctx->config, config_api_invalid, ctx)) < 0) {
    LOG_ERROR("config_valid");
    return err;
  }

  return config_api_validation_done(ctx);
}

static int config_api_write_config_state(struct json_writer *w, void *ctx)
{
  const struct config *config = ctx;
  TickType_t tick = xTaskGetTickCount();

  return JSON_WRITE_OBJECT(w,
        JSON_WRITE_MEMBER_STRING(w, "filename", config->filename)
    ||  JSON_WRITE_MEMBER_STRING(w, "state", config_state_str(config->state))
    ||  JSON_WRITE_MEMBER_UINT(w, "tick", config->tick)
    ||  JSON_WRITE_MEMBER_UINT(w, "tick_ms", TICK_MS(tick, config->tick))
  );
}

/* POST /api/config application/x-www-form-urlencoded */
int config_api_post_form (struct http_request *request, struct http_response *response)
{
  struct config_api_set ctx = { .config = &config, .response = response };
  char *key, *value;
  int err;

  while (!(err = http_request_form(request, &key, &value))) {
    if ((err = config_api_set(&ctx, key, value))) {
      LOG_WARN("config_api_set %s", key);
      return err;
    }
  }

  if (err < 0) {
    LOG_ERROR("http_request_form");
    return err;
  }

  if ((err = config_api_set_done(&ctx)) < 0) {
    LOG_ERROR("config_api_set_done");
    return err;
  } else if (err) {
    LOG_WARN("set errors -> %u", err);
    return err;
  }

  if ((err = config_api_valid(&ctx)) < 0) {
    LOG_ERROR("config_api_valid");
    return err;
  } else if (err) {
    LOG_WARN("valid errors -> %u", err);
    return err;
  } else {
    LOG_INFO("valid");
  }

  LOG_INFO("config save %s", CONFIG_BOOT_FILE);

  if ((err = config_save(ctx.config, CONFIG_BOOT_FILE))) {
    LOG_ERROR("config_save");
    return err;
  }

  if ((err = write_http_response_json(response, config_api_write_config_state, &config))) {
    LOG_WARN("write_http_response_json -> config_api_write_config_state");
    return err;
  }

  
  return 0;
}

int config_api_post (struct http_request *request, struct http_response *response, void *ctx)
{
  const struct http_request_headers *headers;
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_ERROR("http_request_headers");
    return err;
  }

  switch (headers->content_type) {
    case HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED:
      return config_api_post_form(request, response);

    default:
      return http_response_error(response, HTTP_UNSUPPORTED_MEDIA_TYPE, NULL, "Unsupported Content-Type\n");
  }
}
