#include <config.h>

#include <logging.h>

#include <string.h>

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
