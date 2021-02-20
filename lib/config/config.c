#include "config.h"
#include "logging.h"

#include <string.h>

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

int config_set(const struct configmod *mod, const struct configtab *tab, const char *value)
{
  unsigned uvalue;

  if (tab->readonly) {
    LOG_WARN("Config %s.%s is readonly", mod->name, tab->name);
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

    default:
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

    default:
      return -CMD_ERR_NOT_IMPLEMENTED;
  }

  return 0;
}
