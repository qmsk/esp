#include "config.h"
#include "http_routes.h"

#include <logging.h>
#include <json.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_HTTP_FILENAME "config.ini"
#define CONFIG_HTTP_CONTENT_TYPE "text/plain"

int config_get_handler(struct http_request *request, struct http_response *response, void *ctx)
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

int config_post_handler_ini(struct http_request *request, struct http_response *response, void *ctx)
{
  FILE *file;
  int err;

  if ((err = http_request_open(request, &file))) {
    LOG_WARN("http_request_open");
    return err;
  }

  LOG_INFO("config read...");
  LOG_DEBUG("file=%p", file);

  if ((err = config_read(&config, file))) {
    LOG_WARN("config_read");
    err = HTTP_UNPROCESSABLE_ENTITY;
    goto error;
  }

  LOG_INFO("config save...");

  if ((err = config_save(&config))) {
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

int config_post_handler(struct http_request *request, struct http_response *response, void *ctx)
{
  const struct http_request_headers *headers;
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  switch (headers->content_type) {
    case HTTP_CONTENT_TYPE_TEXT_PLAIN:
      return config_post_handler_ini(request, response, ctx);

    default:
      LOG_WARN("Unkonwn Content-Type");

      return HTTP_UNSUPPORTED_MEDIA_TYPE;
  }
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

static int config_api_write_configtab_bool_members(struct json_writer *w, const struct configtab *tab)
{
  return (
    JSON_WRITE_MEMBER_STRING(w, "type", "bool") ||
    JSON_WRITE_MEMBER_OBJECT(w, "value", JSON_WRITE_MEMBER_BOOL(w, "bool", *tab->value.boolean))
  );
}

static int config_api_write_configtab_enum_values(struct json_writer *w, const struct config_enum *e)
{
  int err;

  for (; e->name; e++) {
    if ((err = json_write_string(w, e->name))) {
      return err;
    }
  }

  return 0;
}

static int config_api_write_configtab_enum_members(struct json_writer *w, const struct configtab *tab)
{
  const struct config_enum *e;

  if (config_enum_find_by_value(tab->enum_values, *tab->value.enum_value, &e)) {
    LOG_ERROR("%s: unknown value: %#x", tab->name, *tab->value.enum_value);
    return -1;
  }

  return (
    JSON_WRITE_MEMBER_STRING(w, "type", "enum") ||
    JSON_WRITE_MEMBER_OBJECT(w, "value", JSON_WRITE_MEMBER_STRING(w, "enum", e->name)) ||
    JSON_WRITE_MEMBER_ARRAY(w, "enum_values", config_api_write_configtab_enum_values(w, tab->enum_values))
  );
}

static int config_api_write_configtab_value_members(struct json_writer *w, const struct configtab *tab)
{
  switch (tab->type) {
    case CONFIG_TYPE_UINT16:
      return config_api_write_configtab_uint16_members(w, tab);
    case CONFIG_TYPE_STRING:
      return config_api_write_configtab_string_members(w, tab);
    case CONFIG_TYPE_BOOL:
      return config_api_write_configtab_bool_members(w, tab);
    case CONFIG_TYPE_ENUM:
      return config_api_write_configtab_enum_members(w, tab);
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
  int err = 0;

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
  int err = 0;

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

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

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
    LOG_WARN("no [module]... given");
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  if (!*namep) {
    LOG_WARN("no [...]name given");
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  return 0;
}

static int config_api_set (struct config *config, char *key, char *value)
{
  const char *module, *name;
  const struct configmod *mod;
  const struct configtab *tab;
  int err;

  if ((err = config_api_parse_name(key, &module, &name))) {
    LOG_WARN("config_api_parse_name: %s", key);
    return err;
  }

  if ((err = config_lookup(config, module, name, &mod, &tab))) {
    LOG_WARN("config_lookup: module=%s name=%s", module, name);
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  LOG_INFO("module=%s name=%s: value=%s", module, name, value);

  if ((err = config_set(mod, tab, value))) {
    LOG_WARN("config_set: module=%s name=%s: value=%s", module, name, value);
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  return 0;
}

/* POST /api/config application/x-www-form-urlencoded */
int config_api_post_form(struct http_request *request, struct http_response *response, void *ctx)
{
    char *key, *value;
    int err;

    while (!(err = http_request_form(request, &key, &value))) {
      if ((err = config_api_set(&config, key, value))) {
        LOG_WARN("config_api_set %s", key);
        return err;
      }
    }

    if (err < 0) {
      LOG_ERROR("http_request_form");
      return err;
    }

    LOG_INFO("config save...");

    if ((err = config_save(&config))) {
      LOG_ERROR("config_save");
      return err;
    }

    return HTTP_NO_CONTENT;
}

int config_api_post(struct http_request *request, struct http_response *response, void *ctx)
{
  const struct http_request_headers *headers;
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  switch (headers->content_type) {
    case HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED:
      return config_api_post_form(request, response, ctx);

    default:
      LOG_WARN("Unkonwn Content-Type");

      return HTTP_UNSUPPORTED_MEDIA_TYPE;
  }

}
