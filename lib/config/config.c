#include "config.h"
#include "logging.h"

#include <errno.h>
#include <stdio.h>
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

    default:
      return -CMD_ERR_NOT_IMPLEMENTED;
  }

  return 0;
}

int config_load(struct config *config)
{
  FILE *file;
  int err = 0;

  if ((file = fopen(config->filename, "r")) == NULL) {
    LOG_ERROR("fopen %s: %s", config->filename, strerror(errno));
    return -1;
  }

  LOG_INFO("%s", config->filename);

  if ((err = config_read(config, file))) {
    fclose(file);
    return err;
  }

  fclose(file);

  return err;
}

int config_save(struct config *config)
{
  char newfile[CONFIG_FILENAME];
  FILE *file;
  int err = 0;

  if (snprintf(newfile, sizeof(newfile), "%s.new", config->filename) >= sizeof(newfile)) {
    LOG_ERROR("filename too long: %s.new", config->filename);
    return -1;
  }

  if ((file = fopen(newfile, "w")) == NULL) {
    LOG_ERROR("fopen %s: %s", config->filename, strerror(errno));
    return -1;
  }

  LOG_INFO("%s", newfile);

  if ((err = config_write(config, file))) {
    fclose(file);
    return err;
  }

  if (fclose(file)) {
    LOG_ERROR("fclose %s: %s", newfile, strerror(errno));
    return -1;
  }

  if (remove(config->filename)) {
    LOG_ERROR("remove %s: %s", config->filename, strerror(errno));
    return -1;
  }

  if (rename(newfile, config->filename)) {
    LOG_ERROR("rename %s -> %s: %s", newfile, config->filename, strerror(errno));
    return -1;
  }

  return err;
}
