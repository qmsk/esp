#include <config.h>

#include <logging.h>

#include <string.h>

int config_clear(const struct configmod *mod, const struct configtab *tab)
{
  if (tab->migrated) {
    LOG_WARN("%s.%s: migrated", mod->name, tab->name);
    return -1;
  }

  if (tab->readonly) {
    LOG_WARN("%s.%s: readonly", mod->name, tab->name);
    return -1;
  }

  if (tab->count) {
    // any values previously set will just be ignored, no need to explicitly clear
    *tab->count = 0;
  } else {
    switch (tab->type) {
      case CONFIG_TYPE_STRING:
        memset(tab->string_type.value, '\0', tab->string_type.size);
        break;

      case CONFIG_TYPE_UINT16:
        *tab->uint16_type.value = tab->enum_type.default_value;
        break;

      case CONFIG_TYPE_BOOL:
        *tab->bool_type.value = false;
        break;

      case CONFIG_TYPE_ENUM:
        *tab->enum_type.value = tab->enum_type.default_value;
        break;

      case CONFIG_TYPE_FILE:
        memset(tab->file_type.value, 0, tab->file_type.size);
        break;

      default:
        LOG_ERROR("invalid type=%d", tab->type);
        return -1;
    }
  }

  return 0;
}

static int config_set_string(const struct configmod *mod, const struct configtab *tab, unsigned index, const char *str)
{
  if (tab->migrated) {
    LOG_FATAL("TODO: migrated STRING");
  }
  
  if (snprintf(&tab->string_type.value[index * tab->string_type.size], tab->string_type.size, "%s", str) >= tab->string_type.size) {
    return -1;
  }

  return 0;
}

static int config_set_uint16(const struct configmod *mod, const struct configtab *tab, unsigned index, const char *str)
{
  unsigned value;

  if (sscanf(str, "%u", &value) <= 0) {
    return -1;
  }
  
  if (value > UINT16_MAX) {
    return -1;
  }
  
  if (tab->uint16_type.max && value > tab->uint16_type.max) {
    return -1;
  }
  
  if (tab->migrated) {
    return tab->uint16_type.migrate_func(value, tab->ctx);
  }

  tab->uint16_type.value[index] = (uint16_t) value;

  return 0;
}

static int config_set_bool(const struct configmod *mod, const struct configtab *tab, unsigned index, const char *str)
{
  bool value;

  if (strcmp(str, "true") == 0) {
    value = true;
  } else if (strcmp(str, "false") == 0) {
    value = false;
  } else {
    return -1;
  }

  if (tab->migrated) {
    return tab->bool_type.migrate_func(value, tab->ctx);
  }

  tab->bool_type.value[index] = value;

  return 0;
}

int config_set_enum(const struct configmod *mod, const struct configtab *tab, unsigned index, const char *value)
{
  const struct config_enum *e;

  if (config_enum_lookup(tab->enum_type.values, value, &e)) {
    LOG_WARN("%s.%s: unknown value: %s", mod->name, tab->name, value);
    return -1;
  }

  if (tab->migrated) {
    LOG_FATAL("TODO: migrated ENUM");
  }

  tab->enum_type.value[index] = e->value;

  return 0;
}

int config_set_file(const struct configmod *mod, const struct configtab *tab, unsigned index, const char *value)
{
  int err;
  
  if (tab->migrated) {
    LOG_FATAL("TODO: migrated FILE");
  }
  
  if (!*value) {
    // empty
  } else if ((err = config_file_check(tab->file_type.paths, value)) < 0) {
    LOG_WARN("%s.%s: invalid file: %s", mod->name, tab->name, value);
    return -1;
  } else if (err) {
    LOG_WARN("%s.%s: file not found: %s", mod->name, tab->name, value);
    return -1;
  }

  if (snprintf(&tab->file_type.value[index * tab->file_type.size], tab->file_type.size, "%s", value) >= tab->file_type.size) {
    return -1;
  }

  return 0;
}

int config_set(const struct configmod *mod, const struct configtab *tab, const char *value)
{
  unsigned index;

  if (tab->readonly) {
    LOG_WARN("%s.%s: readonly", mod->name, tab->name);
    return -1;
  }

  if (!tab->count) {
    index = 0;
  } else if (*tab->count < tab->size) {
    index = (*tab->count)++;
  } else {
    LOG_WARN("%s.%s: too many values", mod->name, tab->name);
    return -1;
  }

  if (tab->migrated) {
    LOG_WARN("%s.%s: migrate", mod->name, tab->name);
  }

  LOG_DEBUG("type=%u name=%s index=%u", tab->type, tab->name, index);

  switch (tab->type) {
    case CONFIG_TYPE_STRING:
      return config_set_string(mod, tab, index, value);

    case CONFIG_TYPE_UINT16:
      return config_set_uint16(mod, tab, index, value);

    case CONFIG_TYPE_BOOL:
      return config_set_bool(mod, tab, index, value);

    case CONFIG_TYPE_ENUM:
      return config_set_enum(mod, tab, index, value);

    case CONFIG_TYPE_FILE:
      return config_set_file(mod, tab, index, value);

    default:
      LOG_ERROR("invalid type=%d", tab->type);
      return -1;
  }

  return 0;
}
