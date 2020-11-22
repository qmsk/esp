#include "config.h"
#include "config_cmd.h"
#include "user_config.h"

#include <lib/cli.h>
#include <lib/logging.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int config_read(struct user_config *config)
{
  int fd, ret, err = 0;

  if ((fd = open("config", O_RDONLY)) < 0) {
    LOG_ERROR("open: %d", fd);
    return -1;
  }

  if ((ret = read(fd, config, sizeof(*config))) < 0) {
    err = -1;
    LOG_ERROR("read: %d", ret);
    goto error;
  }

  if (ret < sizeof(*config)) {
    LOG_WARN("read undersize: %d", ret);
    config->version = 0;
  } else {
    LOG_INFO("read", ret);
  }

error:
  close(fd);

  return err;
}

int config_write(struct user_config *config)
{
  int fd, ret, err = 0;

  if ((fd = open("config", O_WRONLY | O_CREAT)) < 0) {
    LOG_ERROR("open: %d", fd);
    return -1;
  }

  if ((ret = write(fd, config, sizeof(*config))) < 0) {
    err = -1;
    LOG_ERROR("write: %d", ret);
    goto error;
  }

  LOG_INFO("%d/%d", ret, sizeof(*config));

error:
  close(fd);

  return err;
}

int config_upgrade(struct user_config *config, const struct user_config *upgrade_config)
{
  LOG_WARN("invalid config version=%d: expected version %d", upgrade_config->version, config->version);

  // TODO: upgrade/downgrade?
  return config_write(config);
}

int config_load(struct user_config *config, const struct user_config *load_config)
{
  *config = *load_config;

  LOG_INFO("version=%d", config->version);

  return 0;
}

int init_config(struct user_config *config)
{
  struct user_config stored_config;
  int err;

  if ((err = config_read(&stored_config))) {
    LOG_WARN("reset config on read error: %d", err);

    return config_write(config);
  } else if (stored_config.version != config->version) {
    return config_upgrade(config, &stored_config);
  } else {
    return config_load(config, &stored_config);
  }
}

// CLI
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

static int configtab_print(const struct configtab *tab)
{
  switch(tab->type) {
    case CONFIG_TYPE_NULL:
      break;

    case CONFIG_TYPE_STRING:
      if (tab->secret) {
        cli_printf("%s = ***\n", tab->name);
      } else {
        cli_printf("%s = %s\n", tab->name, tab->value.string);
      }

      break;

    case CONFIG_TYPE_UINT16:
      if (tab->secret) {
        cli_printf("%s = ***\n", tab->name);
      } else {
        cli_printf("%s = %u\n", tab->name, *tab->value.uint16);
      }

      break;

    default:
      cli_printf("%s = ???\n", tab->name);

      break;
  }

  return 0;
}

static int configmod_print(const struct configmod *mod)
{
  cli_printf("[%s]\n", mod->name);

  for (const struct configtab *tab = mod->table; tab->type && tab->name; tab++) {
    configtab_print(tab);
  }

  cli_printf("\n");

  return 0;
}

static int config_print(const struct config *config)
{
  for (const struct configmod *mod = config->modules; mod->name; mod++) {
    configmod_print(mod);
  }

  return 0;
}

int config_cmd_show(int argc, char **argv, void *ctx)
{
  const struct config *config = ctx;
  const struct configmod *mod;
  const char *section;
  int err;

  if (argc == 2) {
    if ((err = cmd_arg_str(argc, argv, 1, &section))) {
      return err;
    }

    if (configmod_lookup(config->modules, section, &mod)) {
      LOG_ERROR("Unkown config section: %s", section);
      return -CMD_ERR_ARGV;
    }

    configmod_print(mod);
  } else {
    config_print(config);
  }

  return 0;
}

int config_cmd_get(int argc, char **argv, void *ctx)
{
  const struct config *config = ctx;
  const struct configmod *mod;
  const struct configtab *tab;
  const char *section, *name;
  char value[CONFIG_VALUE_SIZE];

  int err;

  if ((err = cmd_arg_str(argc, argv, 1, &section))) {
    return err;
  }
  if ((err = cmd_arg_str(argc, argv, 2, &name))) {
    return err;
  }

  if (config_lookup(config, section, name, &mod, &tab)) {
    LOG_ERROR("Unkown config: %s.%s", section, name);
    return -CMD_ERR_ARGV;
  }

  if (config_get(mod, tab, value, sizeof(value))) {
    LOG_ERROR("Invalid config %s.%s value: %s", mod->name, tab->name);
    return -CMD_ERR;
  }

  cli_printf("%s\n", value);

  return 0;
}

int config_cmd_set(int argc, char **argv, void *ctx)
{
  const struct config *config = ctx;
  const struct configmod *mod;
  const struct configtab *tab;
  const char *section, *name, *value;

  int err;

  if ((err = cmd_arg_str(argc, argv, 1, &section))) {
    return err;
  }
  if ((err = cmd_arg_str(argc, argv, 2, &name))) {
    return err;
  }
  if ((err = cmd_arg_str(argc, argv, 3, &value))) {
    return err;
  }

  if (config_lookup(config, section, name, &mod, &tab)) {
    LOG_ERROR("Unkown config: %s.%s", section, name);
    return -CMD_ERR_ARGV;
  }

  if (config_set(mod, tab, value)) {
    LOG_ERROR("Invalid config %s.%s value: %s", mod->name, tab->name);
    return -CMD_ERR_ARGV;
  }

  if (config_write(&user_config)) {
    LOG_ERROR("Failed writing config");
    return -CMD_ERR;
  }

  return 0;
}

const struct cmd config_commands[] = {
  { "show",              config_cmd_show, .usage = "[SECTION]",           .describe = "Show config settings"  },
  { "get",               config_cmd_get,  .usage = "SECTION NAME",        .describe = "Get config setting"    },
  { "set",               config_cmd_set,  .usage = "SECTION NAME VALUE",  .describe = "Set and write config"  },
  {}
};

const struct cmdtab config_cmdtab = {
  .commands = config_commands,
  .arg      = (void *) &user_configmeta,
};
