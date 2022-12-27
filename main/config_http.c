#include "config.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>
#include <json.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_HTTP_FILENAME "config.ini"
#define CONFIG_HTTP_CONTENT_TYPE "text/plain"

#define CONFIG_FILE_PATH_MAX 64
#define CONFIG_FILE_PATH_PREFIX "/config/"
#define CONFIG_FILE_CONTENT_TYPE "application/octet-stream"

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

static int config_api_write_enum_value(struct json_writer *w, const struct configtab *tab, unsigned index)
{
  const struct config_enum *e;

  if (config_enum_find_by_value(tab->enum_type.values, tab->enum_type.value[index], &e)) {
    LOG_ERROR("%s: unknown value: %#x", tab->name, tab->enum_type.value[index]);
    return -1;
  }

  return json_write_string(w, e->name);
}

static int config_api_write_configtab_value(struct json_writer *w, const struct configtab *tab, unsigned index)
{

  switch(tab->type) {
    case CONFIG_TYPE_UINT16:
      return json_write_uint(w, tab->uint16_type.value[index]);

    case CONFIG_TYPE_STRING:
      return json_write_nstring(w, &tab->string_type.value[index * tab->string_type.size], tab->string_type.size);

    case CONFIG_TYPE_BOOL:
      return json_write_bool(w, tab->bool_type.value[index]);

    case CONFIG_TYPE_ENUM:
      return config_api_write_enum_value(w, tab, index);

    default:
      LOG_ERROR("unknown type=%d", tab->type);
      return -1;
  }

  return 0;
}

static int config_api_write_configtab_values(struct json_writer *w, const struct configtab *tab)
{
  int err;

  for (unsigned count = configtab_count(tab), index = 0; index < count; index++) {
    if ((err = config_api_write_configtab_value(w, tab, index))) {
      return err;
    }
  }

  return 0;
}

static int config_api_write_configtab_type_members(struct json_writer *w, const struct configtab *tab, const char *type)
{
  return (
        JSON_WRITE_MEMBER_STRING(w, "type", type)
    || (tab->count
          ? JSON_WRITE_MEMBER_OBJECT(w, "values", JSON_WRITE_MEMBER_ARRAY(w, type, config_api_write_configtab_values(w, tab)))
          : JSON_WRITE_MEMBER_OBJECT(w, "value", JSON_WRITE_MEMBER(w, type, config_api_write_configtab_value(w, tab, 0)))
       )
  );
}

static int config_api_write_configtab_members_uint16(struct json_writer *w, const struct configtab *tab)
{
  return (
        config_api_write_configtab_type_members(w, tab, "uint16")
    || (tab->uint16_type.max ? JSON_WRITE_MEMBER_UINT(w, "uint16_max", tab->uint16_type.max) : 0)
  );
}
static int config_api_write_configtab_members_string(struct json_writer *w, const struct configtab *tab)
{
  return (
        config_api_write_configtab_type_members(w, tab, "string")
    ||  JSON_WRITE_MEMBER_UINT(w, "string_size", tab->string_type.size)
  );
}

static int config_api_write_configtab_members_bool(struct json_writer *w, const struct configtab *tab)
{
  return (
        config_api_write_configtab_type_members(w, tab, "bool")
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

static int config_api_write_configtab_members_enum(struct json_writer *w, const struct configtab *tab)
{
  return (
        config_api_write_configtab_type_members(w, tab, "enum")
    ||  JSON_WRITE_MEMBER_ARRAY(w, "enum_values", config_api_write_configtab_enum_values(w, tab->enum_type.values))
  );
}

static int config_api_write_configtab_members(struct json_writer *w, const struct configtab *tab)
{
  switch (tab->type) {
    case CONFIG_TYPE_UINT16:
      return config_api_write_configtab_members_uint16(w, tab);
    case CONFIG_TYPE_STRING:
      return config_api_write_configtab_members_string(w, tab);
    case CONFIG_TYPE_BOOL:
      return config_api_write_configtab_members_bool(w, tab);
    case CONFIG_TYPE_ENUM:
      return config_api_write_configtab_members_enum(w, tab);
    default:
      return 0;
  }
}

static int config_api_write_configtab(struct json_writer *w, const struct configtab *tab)
{
  return JSON_WRITE_OBJECT(w,
        JSON_WRITE_MEMBER_STRING(w, "name", tab->name)
    ||  (tab->description ? JSON_WRITE_MEMBER_STRING(w, "description", tab->description) : 0)
    ||  JSON_WRITE_MEMBER_BOOL(w, "readonly", tab->readonly)
    ||  JSON_WRITE_MEMBER_BOOL(w, "secret", tab->secret)
    ||  (tab->count ? JSON_WRITE_MEMBER_UINT(w, "count", *tab->count) : 0)
    ||  (tab->size ? JSON_WRITE_MEMBER_UINT(w, "size", tab->size) : 0)
    ||  config_api_write_configtab_members(w, tab)
  );
}

static int config_api_write_configmod_table(struct json_writer *w, const struct configmod *mod, const struct configtab *table)
{
  int err = 0;

  for (const struct configtab *tab = table; tab->type && tab->name; tab++) {
    if ((err = config_api_write_configtab(w, tab))) {
      LOG_ERROR("config_api_write_configtab: mod=%s tab=%s", mod->name, tab->name);
      return err;
    }
  }

  return err;
}

static int config_api_write_configmod(struct json_writer *w, const struct configmod *mod, int index, const struct configtab *table)
{
  return JSON_WRITE_OBJECT(w,
    JSON_WRITE_MEMBER_STRING(w, "name", mod->name) ||
    (mod->description ? JSON_WRITE_MEMBER_STRING(w, "description", mod->description) : 0) ||
    (index ? JSON_WRITE_MEMBER_INT(w, "index", index) : 0) ||
    JSON_WRITE_MEMBER_ARRAY(w, "table", config_api_write_configmod_table(w, mod, table)) // assume all are the same
  );
}

static int config_api_write_config_modules(struct json_writer *w, const struct config *config)
{
  int err = 0;

  for (const struct configmod *mod = config->modules; mod->name; mod++) {
    if (mod->tables_count) {
      for (int i = 0; i < mod->tables_count; i++) {
        if ((err = config_api_write_configmod(w, mod, i + 1, mod->tables[i]))) {
          LOG_ERROR("config_api_write_configmod: mod=%s", mod->name);
          return err;
        }
      }
    } else {
      if ((err = config_api_write_configmod(w, mod, 0, mod->table))) {
        LOG_ERROR("config_api_write_configmod: mod=%s", mod->name);
        return err;
      }
    }
  }

  return err;
}

static int config_api_write_config(struct json_writer *w, void *ctx)
{
  const struct config *config = ctx;

  return JSON_WRITE_OBJECT(w,
        JSON_WRITE_MEMBER_STRING(w, "filename", CONFIG_BOOT_FILE)
    ||  JSON_WRITE_MEMBER_ARRAY(w, "modules", config_api_write_config_modules(w, config))
  );
}

int config_api_get(struct http_request *request, struct http_response *response, void *ctx)
{
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = write_http_response_json(response, config_api_write_config, &config))) {
    LOG_WARN("write_http_response_json -> config_api_write_config");
    return err;
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

  if (value && *value) {
    LOG_INFO("module=%s name=%s: set value=%s", module, name, value);

    if ((err = config_set(mod, tab, value))) {
      LOG_WARN("config_set: module=%s name=%s: value=%s", module, name, value);
      return HTTP_UNPROCESSABLE_ENTITY;
    }
  } else {
    LOG_INFO("module=%s name=%s: clear", module, name);

    if ((err = config_clear(mod, tab))) {
      LOG_WARN("config_clear: module=%s name=%s", module, name);
      return HTTP_UNPROCESSABLE_ENTITY;
    }
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

    LOG_INFO("config save %s", CONFIG_BOOT_FILE);

    if ((err = config_save(&config, CONFIG_BOOT_FILE))) {
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

struct config_api_file_params {
  const char *path;
};

int config_api_file_params(struct http_request *request, struct config_api_file_params *params)
{
  char *key, *value;
  int err;

  while (!(err = http_request_query(request, &key, &value))) {
    if (strcasecmp(key, "path") == 0) {
      LOG_INFO("path=%s", value);

      params->path = value;
    } else {
      LOG_WARN("Unknown query param key=%s", key);

      return HTTP_UNPROCESSABLE_ENTITY;
    }
  }

  if (!params->path) {
    LOG_WARN("Missing query param path=...");

    return HTTP_UNPROCESSABLE_ENTITY;
  }

  return 0;
}

int config_api_file_open(FILE **filep, const struct config_api_file_params *params, const char *mode)
{
  char path[CONFIG_FILE_PATH_MAX];
  int ret;

  ret = snprintf(path, sizeof(path), "%s%s", CONFIG_FILE_PATH_PREFIX, params->path);

  if (ret < 0) {
    LOG_ERROR("snprintf");
    return -1;
  } else if (ret >= sizeof(path)) {
    LOG_WARN("path len=%d", ret);
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  if ((*filep = fopen(path, mode))) {
    return 0;
  } else if (errno == ENOENT) {
    LOG_WARN("fopen: %s", strerror(errno));
    return HTTP_NOT_FOUND;
  } else {
    LOG_ERROR("fopen: %s", strerror(errno));
    return -1;
  }
}

int config_api_file_copy(FILE *read, FILE *write)
{
  int c;
  size_t size = 0;

  LOG_DEBUG("read=%p, write=%p", read, write);

  while ((c = fgetc(read)) != EOF) {
    if (fputc(c, write) == EOF) {
      LOG_ERROR("fputc: %s", strerror(errno));
      return -1;
    }

    size++;
  }

  LOG_INFO("size=%u", size);

  if (ferror(read)) {
    LOG_ERROR("fgetc: %s", strerror(errno));
    return -1;
  }

  return 0;
}

int config_api_file_read(struct http_response *response, const struct config_api_file_params *params)
{
  FILE *http_file, *file;
  int err;

  LOG_INFO("path=%s", params->path);

  if ((err = config_api_file_open(&file, params, "r")) < 0) {
    LOG_ERROR("config_api_file_open");
    return err;
  } else if (err) {
    LOG_WARN("config_api_file_open: %d", err);
    return err;
  }

  if ((err = http_response_start(response, HTTP_OK, NULL))) {
    LOG_WARN("http_response_start");
    goto error;
  }

  if ((err = http_response_header(response, "Content-Type", "%s", CONFIG_FILE_CONTENT_TYPE))) {
    LOG_WARN("http_response_header");
    goto error;
  }

  if ((err = http_response_header(response, "Content-Disposition", "attachment; filename=\"%s\"", params->path))) {
    LOG_WARN("http_response_header");
    goto error;
  }

  if ((err = http_response_open(response, &http_file))) {
    LOG_WARN("http_response_open");
    goto error;
  }

  if ((err = config_api_file_copy(file, http_file))) {
    LOG_ERROR("config_api_file_copy");
    goto file_error;
  }

file_error:
  if (fclose(http_file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

error:
  if (fclose(file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

  return err;
}

int config_api_file_write(struct http_request *request, const struct config_api_file_params *params)
{
  FILE *http_file, *file;
  int err;

  LOG_INFO("path=%s", params->path);

  if ((err = config_api_file_open(&file, params, "w")) < 0) {
    LOG_ERROR("config_api_file_open");
    return err;
  } else if (err) {
    LOG_WARN("config_api_file_open: %d", err);
    return err;
  }

  if ((err = http_request_open(request, &http_file))) {
    LOG_WARN("http_request_open");
    goto error;
  }

  if ((err = config_api_file_copy(http_file, file))) {
    LOG_ERROR("config_api_file_copy");
    goto file_error;
  }

file_error:
  if (fclose(http_file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

error:
  if (fclose(file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

  return err;
}

int config_api_file_get(struct http_request *request, struct http_response *response, void *ctx)
{
  const struct http_request_headers *headers;
  struct config_api_file_params params = {};
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = config_api_file_params(request, &params))) {
    LOG_WARN("config_api_file_params");
    return err;
  }

  if ((err = config_api_file_read(response, &params))) {
    LOG_WARN("config_api_file_read");
    return err;
  }

  return 0;
}

int config_api_file_post(struct http_request *request, struct http_response *response, void *ctx)
{
  const struct http_request_headers *headers;
  struct config_api_file_params params = {};
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = config_api_file_params(request, &params))) {
    LOG_WARN("config_api_file_params");
    return err;
  }

  if ((err = config_api_file_write(request, &params))) {
    LOG_WARN("config_api_file_write");
    return err;
  }

  return err;
}
