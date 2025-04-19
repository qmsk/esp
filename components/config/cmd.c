#include "config.h"

#include <cmd.h>
#include <logging.h>
#include <stdio.h>

#include <sdkconfig.h>

#include <freertos/task.h>

#define TICK_MS(current_tick, tick) (tick ? (current_tick - tick) * portTICK_RATE_MS : 0)

#if CONFIG_LOG_COLORS
  #define CLI_FMT_RESET "\033[0m"

  #define CLI_FMT_COLOR(code)   "\033[0;" code "m"
  #define CLI_FMT_COLOR_BLACK   "30"
  #define CLI_FMT_COLOR_RED     "31"
  #define CLI_FMT_COLOR_GREEN   "32"
  #define CLI_FMT_COLOR_YELLOW  "33"
  #define CLI_FMT_COLOR_BLUE    "34"
  #define CLI_FMT_COLOR_MAGENTA "35"
  #define CLI_FMT_COLOR_CYAN    "36"
  #define CLI_FMT_COLOR_GREY    "37"
  #define CLI_FMT_COLOR_DEFAULT "39"

  #define CLI_FMT_COMMENT   CLI_FMT_COLOR(CLI_FMT_COLOR_BLUE)
  #define CLI_FMT_SECTION   CLI_FMT_COLOR(CLI_FMT_COLOR_MAGENTA)
  #define CLI_FMT_NAME      CLI_FMT_COLOR(CLI_FMT_COLOR_CYAN)
  #define CLI_FMT_SEP       CLI_FMT_COLOR(CLI_FMT_COLOR_MAGENTA)
  #define CLI_FMT_VALUE     CLI_FMT_COLOR(CLI_FMT_COLOR_DEFAULT)
  #define CLI_FMT_ERROR     CLI_FMT_COLOR(CLI_FMT_COLOR_RED)
  #define CLI_FMT_WARNING   CLI_FMT_COLOR(CLI_FMT_COLOR_YELLOW)
#else
  #define CLI_FMT_RESET ""
  #define CLI_FMT_COMMENT ""
  #define CLI_FMT_SECTION ""
  #define CLI_FMT_NAME ""
  #define CLI_FMT_SEP ""
  #define CLI_FMT_VALUE ""
  #define CLI_FMT_ERROR ""
  #define CLI_FMT_WARNING ""
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

static void print_valid_invalid(const struct config_path path, void *ctx, const char *fmt, ...)
{
  unsigned count = configtab_count(path.tab);
  va_list args;

  if (path.index > 0) {
    printf(CLI_FMT_SEP "[" CLI_FMT_SECTION "%s%d" CLI_FMT_SEP "]" CLI_FMT_RESET, path.mod->name, path.index);
  } else {
    printf(CLI_FMT_SEP "[" CLI_FMT_SECTION "%s" CLI_FMT_SEP "]" CLI_FMT_RESET, path.mod->name);
  }

  printf(" " CLI_FMT_NAME "%s" CLI_FMT_SEP " = " CLI_FMT_VALUE, path.tab->name);

  for (unsigned index = 0; index < count; index++) {
    config_print(path, index, stdout);
    printf(" ");
  }

  printf(CLI_FMT_WARNING "? ");

  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);

  printf(CLI_FMT_RESET "\n");
}

static void print_configtab_invalid(const struct config_path path, void *ctx, const char *fmt, ...)
{
  va_list args;

  printf(CLI_FMT_WARNING "? ");

  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);

  printf(CLI_FMT_RESET "\n");
}

static int print_configtab_file(const struct config_file_path *p, const char *name, void *ctx)
{
  printf(CLI_FMT_COMMENT "#   * %s @ %s\n", name, p->prefix);

  return 0;
}

static void print_configtab(const struct config_path path)
{
  const struct configtab *tab = path.tab;
  unsigned count = configtab_count(tab);
  int err;
  
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

  if (tab->migrated) {
    printf(" ! migrated");
  }

  printf(CLI_FMT_RESET "\n");

  if (tab->description) {
    print_comment(tab->description);
  }

  if (tab->migrated) {
    return;
  }

  if ((err = configtab_valid(path, print_configtab_invalid, NULL)) < 0) {
    LOG_ERROR("configtab_valid");
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

      config_print(path, index, stdout);

      printf(CLI_FMT_RESET "\n");
    }
  }
}

static void print_configmod(const struct configmod *mod, unsigned index, const struct configtab *table)
{
  if (mod->description) {
    print_comment(mod->description);
  }

  if (index > 0) {
    printf(CLI_FMT_SEP "[" CLI_FMT_SECTION "%s%d" CLI_FMT_SEP "]" CLI_FMT_RESET "\n", mod->name, index);
  } else {
    printf(CLI_FMT_SEP "[" CLI_FMT_SECTION "%s" CLI_FMT_SEP "]" CLI_FMT_RESET "\n", mod->name);
  }

  for (const struct configtab *tab = table; tab->type && tab->name; tab++) {
    struct config_path path = { mod, index, tab };

    print_configtab(path);
  }

  printf("\n");
}

static void print_config(const struct config *config)
{
  for (const struct configmod *mod = config->modules; mod->name; mod++) {
    if (mod->tables_count) {
      for (unsigned i = 0; i < mod->tables_count; i++) {
        print_configmod(mod, i, mod->tables[i]);
      }
    } else {
      print_configmod(mod, 0, mod->table);
    }
  }
}

int config_cmd_state(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;
  TickType_t tick = xTaskGetTickCount();

  printf("Config:\n");
  printf("\tPath: %s\n", config->path);
  printf("\tFilename: %s\n", config->filename);
  printf("\tState: %s @ %dms\n", config_state_str(config->state), TICK_MS(tick, config->tick));

  return 0;
}

int config_cmd_save(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;
  const char *filename = CONFIG_BOOT_FILE;
  int err;

  if (argc >= 2 && (err = cmd_arg_str(argc, argv, 1, &filename))) {
    return err;
  }

  if ((err = config_valid(config, print_valid_invalid, NULL)) < 0) {
    LOG_ERROR("config_valid");
    return -1;
  } else if (err) {
    LOG_WARN("config invalid, refusing to save");
    return 1;
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
  unsigned index;
  const struct configtab *table;
  const char *section;
  int err;

  if (argc == 2) {
    if ((err = cmd_arg_str(argc, argv, 1, &section))) {
      return err;
    }

    if (configmod_lookup(config->modules, section, &mod, &index, &table)) {
      LOG_ERROR("Unkown config section: %s", section);
      return -CMD_ERR_ARGV;
    }

    print_configmod(mod, index, table);
  } else {
    print_config(config);
  }

  return 0;
}

int config_cmd_get(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;
  int err;

  struct config_path path;
  const char *section, *name;
  char value[CONFIG_VALUE_SIZE];

  if ((err = cmd_arg_str(argc, argv, 1, &section))) {
    return err;
  }
  if ((err = cmd_arg_str(argc, argv, 2, &name))) {
    return err;
  }

  if (config_lookup(config, section, name, &path)) {
    LOG_ERROR("Unkown config: %s.%s", section, name);
    return -CMD_ERR_ARGV;
  }

  for (unsigned count = configtab_count(path.tab), index = 0; index < count; index++) {
    if (config_get(path, index, value, sizeof(value))) {
      LOG_ERROR("Invalid config %s%d.%s[%u] value: %s", path.mod->name, path.index, path.tab->name, index, value);
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

  struct config_path path;
  const char *section, *name, *value;

  if ((err = cmd_arg_str(argc, argv, 1, &section))) {
    return err;
  }
  if ((err = cmd_arg_str(argc, argv, 2, &name))) {
    return err;
  }

  if (config_lookup(config, section, name, &path)) {
    LOG_ERROR("Unkown config: %s.%s", section, name);
    return -CMD_ERR_ARGV;
  }

  if (path.tab->count) {
    if (config_clear(path)) {
      LOG_ERROR("config_clear %s%d.%s", path.mod->name, path.index, path.tab->name);
      return -CMD_ERR_ARGV;
    }

    for (unsigned index = 0; 3 + index < argc; index++) {
      if ((err = cmd_arg_str(argc, argv, 3 + index, &value))) {
        return err;
      }

      if (config_set(path, value)) {
        LOG_ERROR("Invalid config %s%d.%s value[%u]: %s", path.mod->name, path.index, path.tab->name, index, value);
        return -CMD_ERR_ARGV;
      }
    }
  } else {
    if ((err = cmd_arg_str(argc, argv, 3, &value))) {
      return err;
    }

    if (config_set(path, value)) {
      LOG_ERROR("Invalid config %s%d.%s value: %s", path.mod->name, path.index, path.tab->name, value);
      return -CMD_ERR_ARGV;
    }
  }

  if ((err = configtab_valid(path, print_valid_invalid, NULL)) < 0){
    LOG_ERROR("configtab_valid");
    return err;
  } else if (err) {
    LOG_WARN("config invalid, fix before `config save`");
  } else {
    LOG_INFO("config modified, use `config save` and reboot");
  }

  return 0;
}

int config_cmd_clear(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;
  int err;

  struct config_path path;
  const char *section, *name;

  if ((err = cmd_arg_str(argc, argv, 1, &section))) {
    return err;
  }
  if ((err = cmd_arg_str(argc, argv, 2, &name))) {
    return err;
  }

  if (config_lookup(config, section, name, &path)) {
    LOG_ERROR("Unkown config: %s.%s", section, name);
    return -CMD_ERR_ARGV;
  }

  if (config_clear(path)) {
    LOG_ERROR("config_clear %s%d.%s", path.mod->name, path.index, path.tab->name);
    return -CMD_ERR_ARGV;
  }

  LOG_INFO("config modified, use `config save` and reboot");

  return 0;
}

int config_cmd_reset(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;
  const char *section = NULL, *name = NULL;
  const struct configmod *module = NULL;
  unsigned index;
  const struct configtab *table = NULL;
  int err;

  if (argc >= 2 && (err = cmd_arg_str(argc, argv, 1, &section))) {
    return err;
  }
  if (argc >= 3 && (err = cmd_arg_str(argc, argv, 2, &name))) {
    return err;
  }

  if (section) {
    if (configmod_lookup(config->modules, section, &module, &index, &table)) {
      LOG_ERROR("Unkonwn config section: %s", section);
      return -CMD_ERR_ARGV;
    }
  }

  if (section && name) {
    struct config_path path = { module, index, NULL };

    if (configtab_lookup(module, index, table, name, &path.tab)) {
      LOG_ERROR("Unkonwn config name: %s.%s", section, name);
      return -CMD_ERR_ARGV;
    }

    LOG_INFO("reset [%s] %s...", section, name);

    if (configtab_reset(path)) {
      LOG_ERROR("configtab_reset");
      return -1;
    }
  } else if (section) {
    LOG_INFO("reset [%s]...", section);

    if (configmod_reset(module, index, table)) {
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

int config_cmd_valid(int argc, char **argv, void *ctx)
{
  struct config *config = ctx;
  const char *section = NULL, *name = NULL;
  const struct configmod *module = NULL;
  unsigned index;
  const struct configtab *table = NULL;
  int err, ret = 0;

  if (argc >= 2 && (err = cmd_arg_str(argc, argv, 1, &section))) {
    return err;
  }
  if (argc >= 3 && (err = cmd_arg_str(argc, argv, 2, &name))) {
    return err;
  }

  if (section) {
    if (configmod_lookup(config->modules, section, &module, &index, &table)) {
      LOG_ERROR("Unkonwn config section: %s", section);
      return -CMD_ERR_ARGV;
    }
  }

  if (section && name) {
    struct config_path path = { module, index, NULL };

    if (configtab_lookup(module, index, table, name, &path.tab)) {
      LOG_ERROR("Unkonwn config name: %s.%s", section, name);
      return -CMD_ERR_ARGV;
    }

    if ((err = configtab_valid(path, print_valid_invalid, NULL)) < 0) {
      LOG_ERROR("configtab_valid: %s%d.%s", path.mod->name, path.index, path.tab->name);
      return -1;
    } else if (err) {
      ret = -1;
    }
  } else if (section) {
    if ((err = configmod_valid(module, index, table, print_valid_invalid, NULL)) < 0) {
      LOG_ERROR("configmod_valid: %s%d", module->name, index);
      return -1;
    } else if (err) {
      ret = -1;
    }
  } else {
    if ((err = config_valid(config, print_valid_invalid, NULL)) < 0) {
      LOG_ERROR("config_valid");
      return -1;
    } else if (err) {
      ret = -1;
    }
  }

  return ret;
}

const struct cmd config_commands[] = {
  { "state",             config_cmd_state,  .usage = "",                                .describe = "Show config state"  },

  { "save",              config_cmd_save,   .usage = "[FILE]",                          .describe = "Save config to filesystem"  },
  { "load",              config_cmd_load,   .usage = "[FILE]",                          .describe = "Load config from filesystem"  },
  { "list",              config_cmd_list,   .usage = "",                                .describe = "List configs from filesystem"  },
  { "delete",            config_cmd_delete, .usage = "[FILE]",                          .describe = "Delete config from filesystem" },

  { "show",              config_cmd_show,   .usage = "[SECTION]",                       .describe = "Show config settings"  },
  { "get",               config_cmd_get,    .usage = "SECTION NAME",                    .describe = "Get config setting"    },
  { "set",               config_cmd_set,    .usage = "SECTION NAME VALUE [VALUE ...]",  .describe = "Set config value"  },
  { "clear",             config_cmd_clear,  .usage = "SECTION NAME",                    .describe = "Clear config value"  },
  { "reset",             config_cmd_reset,  .usage = "[SECTION] [NAME]",                .describe = "Reset config values to default"  },
  { "valid",             config_cmd_valid,  .usage = "[SECTION] [NAME]",                .describe = "Check config values are valid"   },
  {}
};
