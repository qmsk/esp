#include "config.h"

#include <cmd.h>
#include <logging.h>
#include <stdio.h>

static void print_comment(const char *description)
{
  // TODO: line-wrapping?
  printf("# %s\n", description);
}

static void print_configtab(const struct configmod *mod, const struct configtab *tab)
{
  if (tab->description) {
    print_comment(tab->description);
  }

  if (tab->secret) {
    printf("%s = ***\n", tab->name);
  } else {
    printf("%s = ", tab->name);
    config_print(mod, tab, stdout);
    printf("\n");
  }
}

static void print_configmod(const struct configmod *mod)
{
  if (mod->description) {
    print_comment(mod->description);
  }

  printf("[%s]\n", mod->name);

  for (const struct configtab *tab = mod->table; tab->type && tab->name; tab++) {
    print_configtab(mod, tab);
  }

  printf("\n");
}

static void print_config(const struct config *config)
{
  for (const struct configmod *mod = config->modules; mod->name; mod++) {
    print_configmod(mod);
  }
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

    print_configmod(mod);
  } else {
    print_config(config);
  }

  return 0;
}

int config_cmd_get(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;
  int err;

  const struct configmod *mod;
  const struct configtab *tab;
  const char *section, *name;
  char value[CONFIG_VALUE_SIZE];

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
    LOG_ERROR("Invalid config %s.%s value: %s", mod->name, tab->name, value);
    return -CMD_ERR;
  }

  printf("%s\n", value);

  return 0;
}

int config_cmd_set(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;
  int err;

  const struct configmod *mod;
  const struct configtab *tab;
  const char *section, *name, *value;

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
    LOG_ERROR("Invalid config %s.%s value: %s", mod->name, tab->name, value);
    return -CMD_ERR_ARGV;
  }

  if (config_save(config)) {
    LOG_ERROR("Failed writing config");
    return -CMD_ERR;
  }

  return 0;
}

int config_cmd_reset(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;

  if (config_clear(config)) {
    LOG_ERROR("Failed clearing config");
    return -CMD_ERR;
  }

  return 0;
}

const struct cmd config_commands[] = {
  { "show",              config_cmd_show, .usage = "[SECTION]",           .describe = "Show config settings"  },
  { "get",               config_cmd_get,  .usage = "SECTION NAME",        .describe = "Get config setting"    },
  { "set",               config_cmd_set,  .usage = "SECTION NAME VALUE",  .describe = "Set and write config"  },
  { "reset",             config_cmd_reset,                                .describe = "Remove stored config and reset to defaults" },
  {}
};
