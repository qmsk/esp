#include "config.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>
#include <json.h>

#include <freertos/task.h>

#define TICK_MS(current_tick, tick) (tick ? (current_tick - tick) * portTICK_RATE_MS : 0)

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
  int err = 0;

  if ((err = JSON_WRITE_MEMBER_STRING(w, "type", type))) {
    return err;
  }

  if (tab->migrated) {
    return 0;
  } else if (tab->count) {
    return JSON_WRITE_MEMBER_OBJECT(w, "values", JSON_WRITE_MEMBER_ARRAY(w, type, config_api_write_configtab_values(w, tab)));
  } else {
    return JSON_WRITE_MEMBER_OBJECT(w, "value", JSON_WRITE_MEMBER(w, type, config_api_write_configtab_value(w, tab, 0)));
  }
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

static int config_api_write_configtab_members(struct json_writer *w, const struct config_path *path)
{
  const struct configtab *tab = path->tab;

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

static void config_api_write_configtab_invalid(const struct config_path path, void *ctx, const char *fmt, ...)
{
  struct json_writer *w = ctx;
  char buf[128];
  int ret;
  va_list va;

  va_start(va, fmt);
  ret = vsnprintf(buf, sizeof(buf), fmt, va);
  va_end(va);

  if (ret  >= sizeof(buf)) {
    // truncated
    buf[sizeof(buf) - 1] = '.';
    buf[sizeof(buf) - 2] = '.';
    buf[sizeof(buf) - 3] = '.';
  }

  json_write_nstring(w, buf, sizeof(buf));
}

static int config_api_write_configtab_valid(struct json_writer *w, const struct config_path *path)
{
  return JSON_WRITE_MEMBER_ARRAY(w, "validation_errors",
    (configtab_valid(*path, config_api_write_configtab_invalid, w) < 0)
  );
}

static int config_api_write_configtab(struct json_writer *w, const struct config_path *path)
{
  const struct configtab *tab = path->tab;
  
  return JSON_WRITE_OBJECT(w,
        JSON_WRITE_MEMBER_STRING(w, "name", tab->name)
    ||  (tab->description ? JSON_WRITE_MEMBER_STRING(w, "description", tab->description) : 0)
    ||  JSON_WRITE_MEMBER_BOOL(w, "readonly", tab->readonly)
    ||  JSON_WRITE_MEMBER_BOOL(w, "secret", tab->secret)
    ||  JSON_WRITE_MEMBER_BOOL(w, "migrated", tab->migrated)
    ||  (tab->count ? JSON_WRITE_MEMBER_UINT(w, "count", *tab->count) : 0)
    ||  (tab->size ? JSON_WRITE_MEMBER_UINT(w, "size", tab->size) : 0)
    ||  config_api_write_configtab_members(w, path)
    ||  config_api_write_configtab_valid(w, path)
  );
}

static int config_api_write_configmod_table(struct json_writer *w, const struct configmod *mod, unsigned index, const struct configtab *table)
{
  int err = 0;

  for (const struct configtab *tab = table; tab->type && tab->name; tab++) {
    struct config_path path = { mod, index, tab };

    if ((err = config_api_write_configtab(w, &path))) {
      LOG_ERROR("config_api_write_configtab: mod=%s tab=%s", mod->name, tab->name);
      return err;
    }
  }

  return err;
}

static int config_api_write_configmod(struct json_writer *w, const struct configmod *mod, unsigned index, const struct configtab *table)
{
  return JSON_WRITE_OBJECT(w,
    JSON_WRITE_MEMBER_STRING(w, "name", mod->name) ||
    (mod->description ? JSON_WRITE_MEMBER_STRING(w, "description", mod->description) : 0) ||
    (index ? JSON_WRITE_MEMBER_INT(w, "index", index) : 0) ||
    JSON_WRITE_MEMBER_ARRAY(w, "table", config_api_write_configmod_table(w, mod, index, table)) // assume all are the same
  );
}

static int config_api_write_config_modules(struct json_writer *w, const struct config *config)
{
  int err = 0;

  for (const struct configmod *mod = config->modules; mod->name; mod++) {
    if (mod->tables_count) {
      for (unsigned i = 0; i < mod->tables_count; i++) {
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
  TickType_t tick = xTaskGetTickCount();

  return JSON_WRITE_OBJECT(w,
        JSON_WRITE_MEMBER_STRING(w, "filename", CONFIG_BOOT_FILE)
    ||  JSON_WRITE_MEMBER_STRING(w, "state", config_state_str(config->state))
    ||  JSON_WRITE_MEMBER_UINT(w, "tick", config->tick)
    ||  JSON_WRITE_MEMBER_UINT(w, "tick_ms", TICK_MS(tick, config->tick))
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
