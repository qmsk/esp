#include "config.h"
#include "http_routes.h"

#include <lib/logging.h>
#include <lib/json.h>

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

static int config_api_write_configtab_uint16_members(struct json_writer *w, const struct configtab *tab)
{
  return (
    JSON_WRITE_MEMBER_STRING(w, "type", "uint16") ||
    JSON_WRITE_MEMBER_OBJECT(w, "value", JSON_WRITE_MEMBER_UINT(w, "uint16", *tab->value.uint16))
  );
}

static int config_api_write_configtab_string_members(struct json_writer *w, const struct configtab *tab)
{
  return (
    JSON_WRITE_MEMBER_STRING(w, "type", "string") ||
    JSON_WRITE_MEMBER_OBJECT(w, "value", JSON_WRITE_MEMBER_STRING(w, "string", tab->value.string))
  );
}

static int config_api_write_configtab_value_members(struct json_writer *w, const struct configtab *tab)
{
  switch (tab->type) {
    case CONFIG_TYPE_UINT16:
      return config_api_write_configtab_uint16_members(w, tab);
    case CONFIG_TYPE_STRING:
      return config_api_write_configtab_string_members(w, tab);
    default:
      return 0;
  }
}

static int config_api_write_configtab(struct json_writer *w, const struct configtab *tab)
{
  return JSON_WRITE_OBJECT(w,
    JSON_WRITE_MEMBER_STRING(w, "name", tab->name) ||
    (tab->size ? JSON_WRITE_MEMBER_UINT(w, "size", tab->size) : 0) ||
    JSON_WRITE_MEMBER_BOOL(w, "readonly", tab->readonly) ||
    JSON_WRITE_MEMBER_BOOL(w, "secret", tab->secret) ||
    config_api_write_configtab_value_members(w, tab)
  );
}

static int config_api_write_configmod_table(struct json_writer *w, const struct configmod *mod)
{
  int err;

  for (const struct configtab *tab = mod->table; tab->type && tab->name; tab++) {
    if ((err = config_api_write_configtab(w, tab))) {
      LOG_ERROR("config_api_write_configtab: mod=%s tab=%s", mod->name, tab->name);
      return err;
    }
  }

  return err;
}

static int config_api_write_configmod(struct json_writer *w, const struct configmod *mod)
{
  return JSON_WRITE_OBJECT(w,
    JSON_WRITE_MEMBER_STRING(w, "name", mod->name) ||
    JSON_WRITE_MEMBER_ARRAY(w, "table", config_api_write_configmod_table(w, mod))
  );
}

static int config_api_write_config_modules(struct json_writer *w, const struct config *config)
{
  int err;

  for (const struct configmod *mod = config->modules; mod->name; mod++) {
    if ((err = config_api_write_configmod(w, mod))) {
      LOG_ERROR("config_api_write_configmod: mod=%s", mod->name);
      return err;
    }
  }

  return err;
}

static int config_api_write_config(struct json_writer *w, const struct config *config)
{
  return JSON_WRITE_OBJECT(w,
    JSON_WRITE_MEMBER_STRING(w, "filename", config->filename) ||
    JSON_WRITE_MEMBER_ARRAY(w, "modules", config_api_write_config_modules(w, config))
  );
}

int config_api_get(struct http_request *request, struct http_response *response, void *ctx)
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

  if ((err = config_api_write_config(&json_writer, &config))) {
    LOG_WARN("config_get_api_write");
    return -1;
  }

  if (fclose(file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

  return 0;
}
