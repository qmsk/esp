#include "config.h"
#include "logging.h"

#include <stdio.h>

static int configtab_write(const struct configmod *mod, const struct configtab *tab, FILE *file)
{
  LOG_DEBUG("type=%u name=%s", tab->type, tab->name);

  if (fprintf(file, "%s = ", tab->name) < 0) {
    return -1;
  }

  if (config_print(mod, tab, file)) {
    return -1;
  }

  if (fprintf(file, "\n") < 0) {
    return -1;
  }

  return 0;
}

static int configmod_write(const struct configmod *mod, FILE *file)
{
  LOG_DEBUG("name=%s", mod->name);

  if (fprintf(file, "[%s]\n", mod->name) < 0) {
    return -1;
  }

  for (const struct configtab *tab = mod->table; tab->name; tab++) {
    if (configtab_write(mod, tab, file)) {
      return -1;
    }
  }

  if (fprintf(file, "\n") < 0) {
    return -1;
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
