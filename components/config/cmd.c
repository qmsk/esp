#include "config.h"

#include <cmd.h>
#include <logging.h>
#include <stdio.h>

static void print_comment(const char *comment)
{
  bool start = true;

  // support linebreaks for multi-line output
  for (const char *c = comment; *c; c++) {
    if (start) {
      printf("# ");
    }

    putchar(*c);

    if (*c == '\n') {
      start = true;
    } else {
      start = false;
    }
  }

  if (!start) {
    printf("\n");
  }
}

static void print_enum_comment(const struct configtab *tab)
{
  printf(" #");

  for (const struct config_enum * e = tab->enum_type.values; e->name; e++) {
    if (e->value == *tab->enum_type.value) {
      printf(" [%s]", e->name);
    } else {
      printf(" %s", e->name);
    }
  }
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

    if (tab->type == CONFIG_TYPE_ENUM) {
      print_enum_comment(tab);
    }

    printf("\n");
  }
}

static void print_configmod(const struct configmod *mod, const struct configtab *table)
{
  if (mod->description) {
    print_comment(mod->description);
  }

  if (mod->tables_count) {
    int index = -1;

    for (int i = 0; i < mod->tables_count; i++) {
      if (mod->tables[i] == table) {
        index = i;
      }
    }

    if (index >= 0) {
      printf("[%s%d]\n", mod->name, index + 1);
    } else {
      printf("[%s???]\n", mod->name);
    }

  } else {
    printf("[%s]\n", mod->name);
  }

  for (const struct configtab *tab = table; tab->type && tab->name; tab++) {
    print_configtab(mod, tab);
  }

  printf("\n");
}

static void print_config(const struct config *config)
{
  for (const struct configmod *mod = config->modules; mod->name; mod++) {
    if (mod->tables_count) {
      for (int i = 0; i < mod->tables_count; i++) {
        print_configmod(mod, mod->tables[i]);
      }
    } else {
      print_configmod(mod, mod->table);
    }
  }
}

int config_cmd_show(int argc, char **argv, void *ctx)
{
  const struct config *config = ctx;
  const struct configmod *mod;
  const struct configtab *table;
  const char *section;
  int err;

  if (argc == 2) {
    if ((err = cmd_arg_str(argc, argv, 1, &section))) {
      return err;
    }

    if (configmod_lookup(config->modules, section, &mod, &table)) {
      LOG_ERROR("Unkown config section: %s", section);
      return -CMD_ERR_ARGV;
    }

    print_configmod(mod, table);
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

int config_cmd_clear(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;
  int err;

  const struct configmod *mod;
  const struct configtab *tab;
  const char *section, *name;

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

  if (config_clear(mod, tab)) {
    LOG_ERROR("config_clear %s.%s", mod->name, tab->name);
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

  if (config_reset(config)) {
    LOG_ERROR("config_reset");
    return -CMD_ERR;
  }

  return 0;
}

const struct cmd config_commands[] = {
  { "show",              config_cmd_show,   .usage = "[SECTION]",           .describe = "Show config settings"  },
  { "get",               config_cmd_get,    .usage = "SECTION NAME",        .describe = "Get config setting"    },
  { "set",               config_cmd_set,    .usage = "SECTION NAME VALUE",  .describe = "Set and write config"  },
  { "clear",             config_cmd_clear,  .usage = "SECTION NAME",        .describe = "Clear and write config"  },
  { "reset",             config_cmd_reset,                                  .describe = "Remove stored config and reset to defaults" },
  {}
};
