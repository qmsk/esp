#include <config.h>

#include <logging.h>

int configtab_valid(const struct configmod *module, unsigned index, const struct configtab *tab)
{
  int err;

  if (!tab->validate_func) {
    return 0;
  }

  if ((err = tab->validate_func(tab->ctx)) < 0) {
    LOG_ERROR("%s%d.%s: failed", module->name, index, tab->name);
    return err;
  } else if (err) {
    LOG_WARN("%s%d.%s: invalid", module->name, index, tab->name);
    return err;
  } else {
    LOG_INFO("%s%d.%s: ok", module->name, index, tab->name);
    return err;
  }

  return 0;
}

int configmod_valid(const struct configmod *module, unsigned index, const struct configtab *table)
{
  int err, ret = 0;

  for (const struct configtab *tab = table; tab->name; tab++) {
    if ((err = configtab_valid(module, index, tab)) < 0) {
      LOG_ERROR("configtab_valid %s%d.%s", module->name, index, tab->name);
      return err;
    } else if (err) {
      ret = err;
    }
  }

  return ret;
}

int config_valid(struct config *config)
{
  int err, ret = 0;

  for (const struct configmod *mod = config->modules; mod->name; mod++) {
    if (mod->tables_count) {
      for (int i = 0; i < mod->tables_count; i++) {
        if ((err = configmod_valid(mod, i + 1, mod->tables[i])) < 0) {
          LOG_ERROR("configmod_valid: %s%d", mod->name, i + 1);
          return err;
        } else if (err) {
          ret = err;
        }
      }
    } else {
      if ((err = configmod_valid(mod, 0, mod->table)) < 0) {
        LOG_ERROR("configmod_valid: %s", mod->name);
        return err;
      } else if (err) {
        ret = err;
      }
    }
  }

  return ret;
}
