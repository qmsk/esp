#include <config.h>

#include <logging.h>

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

int config_enum_lookup(const struct config_enum *e, const char *name, const struct config_enum **enump)
{
  for (; e->name; e++) {
    if (strcmp(e->name, name) == 0) {
      *enump = e;
      return 0;
    }
  }

  return 1;
}

int config_enum_find_by_value(const struct config_enum *e, int value, const struct config_enum **enump)
{
  for (; e->name; e++) {
    if (e->value == value) {
      *enump = e;
      return 0;
    }
  }

  return 1;
}

const char *config_enum_to_string(const struct config_enum *e, int value)
{
  for (; e->name; e++) {
    if (e->value == value) {
      return e->name;
    }
  }

  return NULL;
}

int config_enum_to_value(const struct config_enum *e, const char *name)
{
  for (; e->name; e++) {
    if (strcmp(e->name, name) == 0) {
      return e->value;
    }
  }

  return -1;
}

int configmod_lookup(const struct configmod *modules, const char *name, const struct configmod **modp, const struct configtab **tablep)
{
  for (const struct configmod *mod = modules; mod->name; mod++) {
    if (mod->tables_count) {
      // match name prefix
      const char *suffix;

      if (strncmp(mod->name, name, strlen(mod->name)) == 0) {
        suffix = name + strlen(mod->name);

        LOG_DEBUG("match name=%s -> mod name=%s suffix=%s", name, mod->name, suffix);

      } else if (mod->alias && strncmp(mod->alias, name, strlen(mod->alias)) == 0) {
        suffix = name + strlen(mod->alias);

        LOG_DEBUG("match name=%s -> mod alias=%s suffix=%s ", name, mod->alias, suffix);
        LOG_WARN("deprecated alias %s, renamed to %s", name, mod->name);
      } else {
        continue;
      }

      // decode index
      char *end;
      int index = strtol(suffix, &end, 10);

      if (!*suffix) {
        LOG_WARN("invalid %s, missing suffix", name);
        continue;
      } else if (end == suffix) {
        LOG_WARN("Invalid suffix for name: %s", name);
        continue;
      } else if (*end) {
        LOG_WARN("Invalid suffix for name: %s", name);
        continue;
      }

      if (!index) {
        LOG_WARN("invalid %s, min suffix is 1", name);
        break;
      } else if (index > mod->tables_count) {
        LOG_WARN("invalid %s, max count is %d", name, mod->tables_count);
        break;
      }

      *modp = mod;
      *tablep = mod->tables[index - 1];

      return 0;

    } else {
      if (strcmp(mod->name, name) == 0) {
        LOG_DEBUG("match name=%s -> mod name=%s", name, mod->name);

      } else if (mod->alias && strcmp(mod->alias, name) == 0) {
        LOG_DEBUG("match name=%s -> mod alias=%s", name, mod->alias);
        LOG_WARN("using deprecated alias %s, renamed to %s", name, mod->name);
      } else {
        continue;
      }

      *modp = mod;
      *tablep = mod->table;

      return 0;
    }
  }

  return 1;
}

int configtab_lookup(const struct configtab *table, const char *name, const struct configtab **tabp)
{
  for (const struct configtab *tab = table; tab->type && tab->name; tab++) {
    if (strcmp(tab->name, name) == 0) {
      *tabp = tab;
      return 0;
    }

    if (tab->alias && strcmp(tab->alias, name) == 0) {
      LOG_WARN("using deprecated %s alias -> %s", name, tab->name);

      *tabp = tab;
      return 0;
    }
  }

  return 1;
}

int config_lookup(const struct config *config, const char *module, const char *name, const struct configmod **modp, const struct configtab **tabp)
{
    const struct configtab *table;

    if (configmod_lookup(config->modules, module, modp, &table)) {
      return 1;
    }
    if (configtab_lookup(table, name, tabp)) {
      return 1;
    }

    return 0;
}

int configtab_reset(const struct configtab *tab)
{
  if (tab->count) {
    *tab->count = 0;
  }

  switch (tab->type) {
    case CONFIG_TYPE_STRING:
      if (snprintf(tab->string_type.value, tab->string_type.size, "%s", tab->string_type.default_value ? tab->string_type.default_value : "") >= tab->string_type.size) {
        LOG_ERROR("invalid default_value=%s", tab->string_type.default_value);
        return -1;
      }
      break;

    case CONFIG_TYPE_UINT16:
      *tab->uint16_type.value = tab->uint16_type.default_value;
      break;

    case CONFIG_TYPE_BOOL:
      *tab->bool_type.value = tab->bool_type.default_value;
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

  return 0;
}

int configmod_reset(const struct configmod *module, const struct configtab *table)
{
  int err;

  for (const struct configtab *tab = table; tab->name; tab++) {
    if ((err = configtab_reset(tab))) {
      LOG_ERROR("configtab_reset %s.%s", module->name, tab->name);
      return err;
    }
  }

  return 0;
}

int config_reset(struct config *config)
{
  int err;

  for (const struct configmod *mod = config->modules; mod->name; mod++) {
    if (mod->tables_count) {
      for (int i = 0; i < mod->tables_count; i++) {
        if ((err = configmod_reset(mod, mod->tables[i]))) {
          LOG_ERROR("configmod_reset: %s%d", mod->name, i + 1);
          return err;
        }
      }
    } else {
      if ((err = configmod_reset(mod, mod->table))) {
        LOG_ERROR("configmod_reset: %s", mod->name);
        return err;
      }
    }
  }

  return 0;
}

int config_clear(const struct configmod *mod, const struct configtab *tab)
{
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

int config_set_enum(const struct configmod *mod, const struct configtab *tab, unsigned index, const char *value)
{
  const struct config_enum *e;

  if (config_enum_lookup(tab->enum_type.values, value, &e)) {
    LOG_WARN("%s.%s: unknown value: %s", mod->name, tab->name, value);
    return -1;
  }

  tab->enum_type.value[index] = e->value;

  return 0;
}

int config_set_file(const struct configmod *mod, const struct configtab *tab, unsigned index, const char *value)
{
  int err;

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
  unsigned uvalue;

  if (!tab->count) {
    index = 0;
  } else if (*tab->count < tab->size) {
    index = (*tab->count)++;
  } else {
    LOG_WARN("%s.%s: too many values", mod->name, tab->name);
    return -1;
  }

  LOG_DEBUG("type=%u name=%s index=%u", tab->type, tab->name, index);

  if (tab->readonly) {
    LOG_WARN("%s.%s: readonly", mod->name, tab->name);
    return -1;
  }

  switch (tab->type) {
    case CONFIG_TYPE_STRING:
      if (snprintf(&tab->string_type.value[index * tab->string_type.size], tab->string_type.size, "%s", value) >= tab->string_type.size) {
        return -1;
      } else {
        break;
      }

    case CONFIG_TYPE_UINT16:
      if (sscanf(value, "%u", &uvalue) <= 0) {
        return -1;
      } else if (uvalue > UINT16_MAX) {
        return -1;
      } else if (tab->uint16_type.max && uvalue > tab->uint16_type.max) {
        return -1;
      } else {
        tab->uint16_type.value[index] = (uint16_t) uvalue;
        break;
      }

    case CONFIG_TYPE_BOOL:
      if (strcmp(value, "true") == 0) {
        tab->bool_type.value[index] = true;
      } else if (strcmp(value, "false") == 0) {
        tab->bool_type.value[index] = false;
      } else {
        return -1;
      }

      break;

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

int config_get_enum(const struct configmod *mod, const struct configtab *tab, unsigned index, char *buf, size_t size)
{
  const struct config_enum *e;

  if (config_enum_find_by_value(tab->enum_type.values, tab->enum_type.value[index], &e)) {
    LOG_ERROR("%s.%s: unknown value: %#x", mod->name, tab->name, tab->enum_type.value[index]);
    return -1;
  }

  if (snprintf(buf, size, "%s", e->name) >= size) {
    return -1;
  }

  return 0;
}

int config_get(const struct configmod *mod, const struct configtab *tab, unsigned index, char *buf, size_t size)
{
  if (index >= configtab_count(tab)) {
    LOG_ERROR("invalid index=%u for count=%u", index, configtab_count(tab));
  }

  LOG_DEBUG("type=%u name=%s index=%u", tab->type, tab->name, index);

  switch(tab->type) {
    case CONFIG_TYPE_NULL:
      break;

    case CONFIG_TYPE_UINT16:
      if (snprintf(buf, size, "%u", tab->uint16_type.value[index]) >= size) {
        return -1;
      } else {
        break;
      }

    case CONFIG_TYPE_STRING:
      if (snprintf(buf, size, "%s", &tab->string_type.value[index * tab->string_type.size]) >= size) {
        return -1;
      } else {
        break;
      }

    case CONFIG_TYPE_BOOL:
      if (snprintf(buf, size, "%s", tab->bool_type.value[index] ? "true" : "false") >= size) {
        return -1;
      } else {
        break;
      }

    case CONFIG_TYPE_ENUM:
      return config_get_enum(mod, tab, index, buf, size);

    case CONFIG_TYPE_FILE:
      if (snprintf(buf, size, "%s", &tab->file_type.value[index * tab->file_type.size]) >= size) {
        return -1;
      } else {
        break;
      }

    default:
      LOG_ERROR("invalid type=%d", tab->type);
      return -1;
  }

  return 0;
}

static int config_print_enum(const struct configmod *mod, const struct configtab *tab, unsigned index, FILE *file)
{
  const struct config_enum *e;

  if (config_enum_find_by_value(tab->enum_type.values, tab->enum_type.value[index], &e)) {
    LOG_ERROR("%s.%s: unknown value: %#x", mod->name, tab->name, tab->enum_type.value[index]);
    return -1;
  }

  LOG_DEBUG("%s.%s: value=%#x enum=%s", mod->name, tab->name, tab->enum_type.value[index], e->name);

  if (fprintf(file, "%s", e->name) < 0) {
    return -1;
  }

  return 0;
}

int config_print(const struct configmod *mod, const struct configtab *tab, unsigned index, FILE *file)
{
  if (index >= configtab_count(tab)) {
    LOG_ERROR("invalid index=%u for count=%u", index, configtab_count(tab));
  }

  LOG_DEBUG("type=%u name=%s index=%u", tab->type, tab->name, index);

  switch (tab->type) {
  case CONFIG_TYPE_NULL:
    break;

  case CONFIG_TYPE_UINT16:
    if (fprintf(file, "%u", tab->uint16_type.value[index]) < 0) {
      return -1;
    }
    break;

  case CONFIG_TYPE_STRING:
    if (fprintf(file, "%s", &tab->string_type.value[index * tab->string_type.size]) < 0) {
      return -1;
    }
    break;

  case CONFIG_TYPE_BOOL:
    if (fprintf(file, "%s", tab->bool_type.value[index] ? "true" : "false") < 0) {
      return -1;
    }
    break;

  case CONFIG_TYPE_ENUM:
    return config_print_enum(mod, tab, index, file);

  case CONFIG_TYPE_FILE:
    if (fprintf(file, "%s", &tab->file_type.value[index * tab->file_type.size]) < 0) {
      return -1;
    }
    break;

  default:
    LOG_ERROR("invalid type=%d", tab->type);
    return -1;
  }

  return 0;
}

int config_init(struct config *config)
{
  return config_reset(config);
}
