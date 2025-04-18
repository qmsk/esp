#include <config.h>

#include <logging.h>

#include <string.h>

int configtab_reset(const struct configtab *tab)
{
  if (tab->migrated) {
    return 0;
  }
  
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
