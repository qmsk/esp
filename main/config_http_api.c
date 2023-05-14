#include "config.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>
#include <json.h>

static int config_api_write_enum_value(struct json_writer *w, const struct configtab *tab, unsigned index)
{
  const struct config_enum *e;

  if (config_enum_find_by_value(tab->enum_type.values, tab->enum_type.value[index], &e)) {
    LOG_ERROR("%s: unknown value: %#x", tab->name, tab->enum_type.value[index]);
    return -1;
  }

  return json_write_string(w, e->name);
}

static int config_api_write_file_value(struct json_writer *w, const struct configtab *tab, unsigned index)
{
  if (tab->file_type.value[index * tab->file_type.size]) {
    return json_write_nstring(w, &tab->file_type.value[index * tab->file_type.size], tab->file_type.size);
  } else {
    return json_write_null(w);
  }
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

    case CONFIG_TYPE_FILE:
      return config_api_write_file_value(w, tab, index);

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

static int config_api_write_configtab_file_paths(struct json_writer *w, const struct config_file_path *p)
{
  int err;

  for (; p->prefix; p++) {
    if (p->suffix) {
      err |= json_write_raw(w, "\"%s/*.%s\"", p->prefix, p->suffix);
    } else {
      err |= json_write_raw(w, "\"%s/*\"", p->prefix);
    }
  }

  return 0;
}

static int config_api_write_configtab_file_value(const struct config_file_path *p, const char *name, void *ctx)
{
  struct json_writer *w = ctx;

  return json_write_string(w, name);
}

static int config_api_write_configtab_file_values(struct json_writer *w, const struct config_file_path *p)
{
  return config_file_walk(p, config_api_write_configtab_file_value, w);
}

static int config_api_write_configtab_members_enum(struct json_writer *w, const struct configtab *tab)
{
  return (
        config_api_write_configtab_type_members(w, tab, "enum")
    ||  JSON_WRITE_MEMBER_ARRAY(w, "enum_values", config_api_write_configtab_enum_values(w, tab->enum_type.values))
  );
}

static int config_api_write_configtab_members_file(struct json_writer *w, const struct configtab *tab)
{
  return (
        config_api_write_configtab_type_members(w, tab, "file")
    ||  JSON_WRITE_MEMBER_ARRAY(w, "file_paths", config_api_write_configtab_file_paths(w, tab->file_type.paths))
    ||  JSON_WRITE_MEMBER_ARRAY(w, "file_values", config_api_write_configtab_file_values(w, tab->file_type.paths))
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
    case CONFIG_TYPE_FILE:
      return config_api_write_configtab_members_file(w, tab);
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
