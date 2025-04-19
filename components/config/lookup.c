#include <config.h>

#include <logging.h>

#include <string.h>

int configmod_lookup(const struct configmod *modules, const char *name, const struct configmod **modp, unsigned *indexp, const struct configtab **tablep)
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

      if (index < 0) {
        LOG_WARN("invalid %s, suffix has dash", name);
        break;
      } else if (!index) {
        LOG_WARN("invalid %s, min suffix is 1", name);
        break;
      } else if (index > mod->tables_count) {
        LOG_WARN("invalid %s, max count is %d", name, mod->tables_count);
        break;
      }

      *modp = mod;
      *indexp = (unsigned) index;
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
      *indexp = 0;
      *tablep = mod->table;

      return 0;
    }
  }

  return 1;
}

int configtab_lookup(const struct configmod *module, unsigned index, const struct configtab *table, const char *name, const struct configtab **tabp)
{
  for (const struct configtab *tab = table; tab->type && tab->name; tab++) {
    if (strcmp(tab->name, name) == 0) {
      *tabp = tab;
      return 0;
    }

    if (tab->alias && strcmp(tab->alias, name) == 0) {
      LOG_WARN("using deprecated %s%u.%s alias -> %s", module->name, index,tab->name, name);

      *tabp = tab;
      return 0;
    }
  }

  return 1;
}

int config_lookup(const struct config *config, const char *module, const char *name, struct config_path *path)
{
    const struct configtab *table;

    if (configmod_lookup(config->modules, module, &path->mod, &path->index, &table)) {
      return 1;
    }
    if (configtab_lookup(path->mod, path->index, table, name, &path->tab)) {
      return 1;
    }

    return 0;
}
