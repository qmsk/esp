#include "config.h"
#include "logging.h"

#include <stdio.h>

static int configtab_write(const struct config_path path, FILE *file)
{
  const struct configtab *tab = path.tab;
  unsigned count = configtab_count(tab);

  if (tab->migrated) {
    LOG_DEBUG("type=%u name=%s migrate", tab->type, tab->name);
    return 0;
  }

  LOG_DEBUG("type=%u name=%s count=%u", tab->type, tab->name, count);

  for (unsigned index = 0; index < count; index++) {
    if (fprintf(file, "%s = ", tab->name) < 0) {
      return -1;
    }

    if (config_print(path, index, file)) {
      return -1;
    }

    if (fprintf(file, "\n") < 0) {
      return -1;
    }
  }

  return 0;
}

static int configmod_write(const struct configmod *mod, FILE *file)
{
  if (mod->tables_count) {
    LOG_DEBUG("name=%s count=%u", mod->name, mod->tables_count);

    for (int i = 0; i < mod->tables_count; i++) {
      if (fprintf(file, "[%s%d]\n", mod->name, i + 1) < 0) {
        return -1;
      }

      for (const struct configtab *tab = mod->tables[i]; tab->name; tab++) {
        struct config_path path = { mod, i + 1, tab };

        if (configtab_write(path, file)) {
          return -1;
        }
      }

      if (fprintf(file, "\n") < 0) {
        return -1;
      }

    }
  } else {
    LOG_DEBUG("name=%s", mod->name);

    if (fprintf(file, "[%s]\n", mod->name) < 0) {
      return -1;
    }

    for (const struct configtab *tab = mod->table; tab->name; tab++) {
      struct config_path path = { mod, 0, tab };

      if (configtab_write(path, file)) {
        return -1;
      }
    }

    if (fprintf(file, "\n") < 0) {
      return -1;
    }
  }

  return 0;
}

int config_write(struct config *config, FILE *file)
{
  for (const struct configmod *mod = config->modules; mod->name; mod++) {
    if (configmod_write(mod, file)) {
      return -1;
    }
  }

  return 0;
}
