#include "config.h"

#include <cmd.h>
#include <logging.h>
#include <stdio.h>

#include <sdkconfig.h>

#if CONFIG_LOG_COLORS
  #define CLI_FMT_RESET "\033[0m"

  #define CLI_FMT_COLOR(code)   "\033[0;" code "m"
  #define CLI_FMT_COLOR_YELLOW  "33"
  #define CLI_FMT_COLOR_BLUE    "34"
  #define CLI_FMT_COLOR_MAGENTA "35"
  #define CLI_FMT_COLOR_CYAN    "36"
  #define CLI_FMT_COLOR_DEFAULT "39"

  #define CLI_FMT_COMMENT   CLI_FMT_COLOR(CLI_FMT_COLOR_BLUE)
  #define CLI_FMT_SECTION   CLI_FMT_COLOR(CLI_FMT_COLOR_MAGENTA)
  #define CLI_FMT_NAME      CLI_FMT_COLOR(CLI_FMT_COLOR_CYAN)
  #define CLI_FMT_SEP       CLI_FMT_COLOR(CLI_FMT_COLOR_YELLOW)
  #define CLI_FMT_VALUE     CLI_FMT_COLOR(CLI_FMT_COLOR_DEFAULT)
#else
  #define CLI_FMT_RESET ""
  #define CLI_FMT_COMMENT ""
  #define CLI_FMT_SECTION ""
  #define CLI_FMT_NAME ""
  #define CLI_FMT_SEP ""
  #define CLI_FMT_VALUE ""
#endif

static void print_comment(const char *comment)
{
  bool start = true;

  // support linebreaks for multi-line output
  for (const char *c = comment; *c; c++) {
    if (start) {
      printf(CLI_FMT_COMMENT "#\t");
    }

    putchar(*c);

    if (*c == '\n') {
      start = true;
    } else {
      start = false;
    }
  }

  if (!start) {
    printf(CLI_FMT_RESET "\n");
  }
}

static int print_configtab_file(const struct config_file_path *p, const char *name, void *ctx)
{
  printf(CLI_FMT_COMMENT "#   * %s @ %s\n", name, p->prefix);

  return 0;
}

static void print_configtab(const struct configmod *mod, const struct configtab *tab)
{
  unsigned count = configtab_count(tab);

  if (tab->count) {
    printf(CLI_FMT_COMMENT "# %s[%u/%u] = ", tab->name, count, tab->size);
  } else {
    printf(CLI_FMT_COMMENT "# %s = ", tab->name);
  }

  switch(tab->type) {
    case CONFIG_TYPE_UINT16:
      printf("<UINT16>[0..%u]", tab->uint16_type.max ? tab->uint16_type.max : UINT16_MAX);
      break;

    case CONFIG_TYPE_STRING:
      printf("<STRING>[%u]", tab->string_type.size);
      break;

    case CONFIG_TYPE_BOOL:
      printf("true | false");
      break;

    case CONFIG_TYPE_ENUM:
      for (const struct config_enum *e = tab->enum_type.values; e->name; e++) {
        if (e == tab->enum_type.values) {
          printf("%s", e->name);
        } else {
          printf(" | %s", e->name);
        }
      }
      break;

    case CONFIG_TYPE_FILE:
      printf("<FILE>[");
      for (const struct config_file_path *p = tab->file_type.paths; p->prefix; p++) {
        if (p != tab->file_type.paths) {
          printf(",");
        }

        if (p->suffix) {
          printf("%s/*.%s", p->prefix, p->suffix);
        } else {
          printf("%s/*", p->prefix);
        }
      }
      printf("]");

      break;

    default:
      printf("???");
      break;
  }

  printf(CLI_FMT_RESET "\n");

  if (tab->description) {
    print_comment(tab->description);
  }

  switch(tab->type) {
    case CONFIG_TYPE_FILE:
      config_file_walk(tab->file_type.paths, print_configtab_file, NULL);
      break;

    default:
      break;
  }

  for (unsigned index = 0; index < count; index++) {
    if (tab->secret) {
      printf(CLI_FMT_NAME "%s" CLI_FMT_SEP " = " CLI_FMT_VALUE "***" CLI_FMT_RESET "\n", tab->name);
    } else {
      printf(CLI_FMT_NAME "%s" CLI_FMT_SEP " = " CLI_FMT_VALUE, tab->name);

      config_print(mod, tab, index, stdout);

      printf(CLI_FMT_RESET "\n");
    }
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
      printf(CLI_FMT_SEP "[" CLI_FMT_SECTION "%s%d" CLI_FMT_SEP "]" CLI_FMT_RESET "\n", mod->name, index + 1);
    } else {
      printf(CLI_FMT_SEP "[" CLI_FMT_SECTION "%s???" CLI_FMT_SEP "]" CLI_FMT_RESET "\n", mod->name);
    }

  } else {
    printf(CLI_FMT_SEP "[" CLI_FMT_SECTION "%s" CLI_FMT_SEP "]" CLI_FMT_RESET "\n", mod->name);
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

int config_cmd_save(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;
  const char *filename = CONFIG_BOOT_FILE;
  int err;

  if (argc >= 2 && (err = cmd_arg_str(argc, argv, 1, &filename))) {
    return err;
  }

  if (config_save(config, filename)) {
    LOG_ERROR("config_save %s", filename);
    return -CMD_ERR;
  }

  return 0;
}

int config_cmd_load(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;
  const char *filename = CONFIG_BOOT_FILE;
  int err;

  if (argc >= 2 && (err = cmd_arg_str(argc, argv, 1, &filename))) {
    return err;
  }

  if (config_load(config, filename)) {
    LOG_ERROR("config_load %s", filename);
    return -CMD_ERR;
  }

  LOG_WARN("config modified, use `config save` and reboot");

  return 0;
}

static int print_config_file(const char *filename, void *ctx)
{
  printf("%s\n", filename);

  return 0;
}

int config_cmd_list(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;

  if (config_walk(config, print_config_file, NULL)) {
    LOG_ERROR("config_walk");
    return -CMD_ERR;
  }

  return 0;
}

int config_cmd_delete(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;
  const char *filename = CONFIG_BOOT_FILE;
  int err;

  if (argc >= 2 && (err = cmd_arg_str(argc, argv, 1, &filename))) {
    return err;
  }

  if (config_delete(config, filename)) {
    LOG_ERROR("config_delete %s", filename);
    return -CMD_ERR;
  }

  return 0;
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

  for (unsigned count = configtab_count(tab), index = 0; index < count; index++) {
    if (config_get(mod, tab, index, value, sizeof(value))) {
      LOG_ERROR("Invalid config %s.%s[%u] value: %s", mod->name, tab->name, index, value);
      return -CMD_ERR;
    }

    printf("%s\n", value);
  }

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

  if (config_lookup(config, section, name, &mod, &tab)) {
    LOG_ERROR("Unkown config: %s.%s", section, name);
    return -CMD_ERR_ARGV;
  }

  if (tab->count) {
    if (config_clear(mod, tab)) {
      LOG_ERROR("config_clear %s.%s", mod->name, tab->name);
      return -CMD_ERR_ARGV;
    }

    for (unsigned index = 0; 3 + index < argc; index++) {
      if ((err = cmd_arg_str(argc, argv, 3 + index, &value))) {
        return err;
      }

      if (config_set(mod, tab, value)) {
        LOG_ERROR("Invalid config %s.%s value[%u]: %s", mod->name, tab->name, index, value);
        return -CMD_ERR_ARGV;
      }
    }
  } else {
    if ((err = cmd_arg_str(argc, argv, 3, &value))) {
      return err;
    }

    if (config_set(mod, tab, value)) {
      LOG_ERROR("Invalid config %s.%s value: %s", mod->name, tab->name, value);
      return -CMD_ERR_ARGV;
    }
  }

  LOG_WARN("config modified, use `config save` and reboot");

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

  LOG_WARN("config modified, use `config save` and reboot");

  return 0;
}

int config_cmd_reset(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;
  const char *section = NULL, *name = NULL;
  const struct configmod *module = NULL;
  const struct configtab *table = NULL;
  int err;

  if (argc >= 2 && (err = cmd_arg_str(argc, argv, 1, &section))) {
    return err;
  }
  if (argc >= 3 && (err = cmd_arg_str(argc, argv, 2, &name))) {
    return err;
  }

  if (section) {
    if (configmod_lookup(config->modules, section, &module, &table)) {
      LOG_ERROR("Unkonwn config section: %s", section);
      return -CMD_ERR_ARGV;
    }
  }

  if (section && name) {
    const struct configtab *tab = NULL;

    if (configtab_lookup(table, name, &tab)) {
      LOG_ERROR("Unkonwn config name: %s.%s", section, name);
      return -CMD_ERR_ARGV;
    }

    LOG_INFO("reset [%s] %s...", section, name);

    if (configtab_reset(tab)) {
      LOG_ERROR("configtab_reset");
      return -1;
    }
  } else if (section) {
    LOG_INFO("reset [%s]...", section);

    if (configmod_reset(module, table)) {
      LOG_ERROR("configmod_reset");
      return -1;
    }
  } else {
    LOG_INFO("reset...");

    if (config_reset(config)) {
      LOG_ERROR("config_reset");
      return -1;
    }
  }

  LOG_WARN("config modified, use `config save` and reboot");

  return 0;
}

const struct cmd config_commands[] = {
  { "save",              config_cmd_save,   .usage = "[FILE]",                          .describe = "Save config to filesystem"  },
  { "load",              config_cmd_load,   .usage = "[FILE]",                          .describe = "Load config from filesystem"  },
  { "list",              config_cmd_list,   .usage = "",                                .describe = "List configs from filesystem"  },
  { "delete",            config_cmd_delete, .usage = "[FILE]",                          .describe = "Delete config from filesystem" },

  { "show",              config_cmd_show,   .usage = "[SECTION]",                       .describe = "Show config settings"  },
  { "get",               config_cmd_get,    .usage = "SECTION NAME",                    .describe = "Get config setting"    },
  { "set",               config_cmd_set,    .usage = "SECTION NAME VALUE [VALUE ...]",  .describe = "Set config value"  },
  { "clear",             config_cmd_clear,  .usage = "SECTION NAME",                    .describe = "Clear config value"  },
  { "reset",             config_cmd_reset,  .usage = "[SECTION] [NAME]",                .describe = "Reset config values to default"  },
  {}
};
