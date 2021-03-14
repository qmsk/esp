#include "config.h"
#include "logging.h"

#include <string.h>

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

int configmod_lookup(const struct configmod *mod, const char *name, const struct configmod **modp)
{
  for (; mod->name; mod++) {
    if (strcmp(mod->name, name) == 0) {
      *modp = mod;
      return 0;
    }
  }

  return 1;
}

int configtab_lookup(const struct configtab *tab, const char *name, const struct configtab **tabp)
{
  for (; tab->type && tab->name; tab++) {
    if (strcmp(tab->name, name) == 0) {
      *tabp = tab;
      return 0;
    }
  }

  return 1;
}

int config_lookup(const struct config *config, const char *module, const char *name, const struct configmod **modp, const struct configtab **tabp)
{
    if (configmod_lookup(config->modules, module, modp)) {
      return 1;
    }
    if (configtab_lookup((*modp)->table, name, tabp)) {
      return 1;
    }

    return 0;
}

int config_set_enum(const struct configmod *mod, const struct configtab *tab, const char *value)
{
  const struct config_enum *e;

  if (config_enum_lookup(tab->enum_values, value, &e)) {
    LOG_WARN("%s.%s: unknown value: %s", mod->name, tab->name, value);
    return -1;
  }

  *tab->value.enum_value = e->value;

  return 0;
}

int config_set(const struct configmod *mod, const struct configtab *tab, const char *value)
{
  unsigned uvalue;

  if (tab->readonly) {
    LOG_WARN("%s.%s: readonly", mod->name, tab->name);
    return -1;
  }

  switch (tab->type) {
    case CONFIG_TYPE_STRING:
      if (snprintf(tab->value.string, tab->size, "%s", value) >= tab->size) {
        return -1;
      } else {
        break;
      }

    case CONFIG_TYPE_UINT16:
      if (sscanf(value, "%u", &uvalue) <= 0) {
        return -1;
      } else if (uvalue > UINT16_MAX) {
        return -1;
      } else {
        *tab->value.uint16 = (uint16_t) uvalue;
        break;
      }

    case CONFIG_TYPE_BOOL:
      if (strcmp(value, "true") == 0) {
        *tab->value.boolean = true;
      } else if (strcmp(value, "false") == 0) {
        *tab->value.boolean = false;
      } else {
        return -1;
      }

      break;

    case CONFIG_TYPE_ENUM:
      return config_set_enum(mod, tab, value);

    default:
      return -1;
  }

  return 0;
}

int config_get_enum(const struct configmod *mod, const struct configtab *tab, char *buf, size_t size)
{
  const struct config_enum *e;

  if (config_enum_find_by_value(tab->enum_values, *tab->value.enum_value, &e)) {
    LOG_ERROR("%s.%s: unknown value: %#x", mod->name, tab->name, *tab->value.enum_value);
    return -1;
  }

  if (snprintf(buf, size, "%s", e->name) >= size) {
    return -1;
  }

  return 0;
}

int config_get(const struct configmod *mod, const struct configtab *tab, char *buf, size_t size)
{
  switch(tab->type) {
    case CONFIG_TYPE_NULL:
      break;

    case CONFIG_TYPE_STRING:
      if (snprintf(buf, size, "%s", tab->value.string) >= size) {
        return -1;
      } else {
        break;
      }

    case CONFIG_TYPE_UINT16:
      if (snprintf(buf, size, "%u", *tab->value.uint16) >= size) {
        return -1;
      } else {
        break;
      }

    case CONFIG_TYPE_BOOL:
      if (snprintf(buf, size, "%s", *tab->value.boolean ? "true" : "false") >= size) {
        return -1;
      } else {
        break;
      }

    case CONFIG_TYPE_ENUM:
      return config_get_enum(mod, tab, buf, size);

    default:
      return -CMD_ERR_NOT_IMPLEMENTED;
  }

  return 0;
}

static int config_print_enum(const struct configmod *mod, const struct configtab *tab, FILE *file)
{
  const struct config_enum *e;

  if (config_enum_find_by_value(tab->enum_values, *tab->value.enum_value, &e)) {
    LOG_ERROR("%s.%s: unknown value: %#x", mod->name, tab->name, *tab->value.enum_value);
    return -1;
  }

  LOG_DEBUG("%s.%s: value=%#x enum=%s", mod->name, tab->name, *tab->value.enum_value, e->name);

  if (fprintf(file, "%s", e->name) < 0) {
    return -1;
  }

  return 0;
}

int config_print(const struct configmod *mod, const struct configtab *tab, FILE *file)
{
  LOG_DEBUG("type=%u name=%s", tab->type, tab->name);

  switch (tab->type) {
  case CONFIG_TYPE_NULL:
    break;

  case CONFIG_TYPE_UINT16:
    if (fprintf(file, "%u", *tab->value.uint16) < 0) {
      return -1;
    }
    break;

  case CONFIG_TYPE_STRING:
    if (fprintf(file, "%s", tab->value.string) < 0) {
      return -1;
    }
    break;

  case CONFIG_TYPE_BOOL:
    if (fprintf(file, "%s", *tab->value.boolean ? "true" : "false") < 0) {
      return -1;
    }
    break;

  case CONFIG_TYPE_ENUM:
    return config_print_enum(mod, tab, file);

  default:
    return -1;
  }

  return 0;
}
