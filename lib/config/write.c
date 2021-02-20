#include "config.h"
#include "logging.h"

#include <stdio.h>

static int configtab_write(const struct configtab *tab, FILE *file)
{
  LOG_DEBUG("type=%u name=%s", tab->type, tab->name);

  switch (tab->type) {
  case CONFIG_TYPE_NULL:
    break;

  case CONFIG_TYPE_UINT16:
    if (fprintf(file, "%s = %u\n", tab->name, *tab->value.uint16) < 0) {
      return -1;
    }
    break;

  case CONFIG_TYPE_STRING:
    if (fprintf(file, "%s = %s\n", tab->name, tab->value.string) < 0) {
      return -1;
    }
    break;

  case CONFIG_TYPE_BOOL:
    if (fprintf(file, "%s = %s\n", tab->name, *tab->value.boolean ? "true" : "false") < 0) {
      return -1;
    }
    break;

  default:
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
    if (configtab_write(tab, file)) {
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
