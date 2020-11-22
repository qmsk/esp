#include "cli.h"
#include "config.h"
#include "logging.h"

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
  for (const struct configtab *tab = mod->table; tab->type && tab->name; tab++) {
    configtab_print(tab);
  }

  return 0;
}

static int config_print(const struct config *config)
{
  for (const struct configmod *mod = config->modules; mod->name; mod++) {
    cli_printf("[%s]\n", mod->name);

    configmod_print(mod);

    cli_printf("\n");
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
    LOG_ERROR("Invalid config %s.%s value: %s", mod->name, tab->name);
    return -CMD_ERR;
  }

  cli_printf("%s\n", value);

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
    LOG_ERROR("Invalid config %s.%s value: %s", mod->name, tab->name);
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
