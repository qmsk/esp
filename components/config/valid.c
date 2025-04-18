#include <config.h>

#include <logging.h>

int configtab_valid(const struct config_path path, config_invalid_handler_t *handler, void *ctx)
{
  const struct configtab *tab = path.tab;
  int err;

  if (!tab->validate_func) {
    return 0;
  }

  if ((err = tab->validate_func(handler, path, ctx)) < 0) {
    LOG_DEBUG("%s%d.%s: failed", path.mod->name, path.index, path.tab->name);
  } else if (err) {
    LOG_DEBUG("%s%d.%s: invalid", path.mod->name, path.index, path.tab->name);
  } else {
    LOG_DEBUG("%s%d.%s: ok", path.mod->name, path.index, path.tab->name);
  }

  return err;
}

int configmod_valid(const struct configmod *module, unsigned index, const struct configtab *table, config_invalid_handler_t handler, void *ctx)
{
  int err, ret = 0;

  for (const struct configtab *tab = table; tab->name; tab++) {
    struct config_path path = { module, index, tab };

    if ((err = configtab_valid(path, handler, ctx)) < 0) {
      LOG_ERROR("%s%d.%s", module->name, index, tab->name);
      return err;
    } else if (err) {
      ret = err;
    }
  }

  return ret;
}

int config_valid(struct config *config, config_invalid_handler_t handler, void *ctx)
{
  int err, ret = 0;

  for (const struct configmod *mod = config->modules; mod->name; mod++) {
    if (mod->tables_count) {
      for (int i = 0; i < mod->tables_count; i++) {
        if ((err = configmod_valid(mod, i + 1, mod->tables[i], handler, ctx)) < 0) {
          LOG_ERROR("configmod_valid: %s%d", mod->name, i + 1);
          return err;
        } else if (err) {
          ret = err;
        }
      }
    } else {
      if ((err = configmod_valid(mod, 0, mod->table, handler, ctx)) < 0) {
        LOG_ERROR("configmod_valid: %s", mod->name);
        return err;
      } else if (err) {
        ret = err;
      }
    }
  }

  return ret;
}
