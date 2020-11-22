#include "config.h"
#include "config_cmd.h"
#include "user_config.h"

#include <lib/cli.h>
#include <lib/logging.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#define ISSPACE(c) (isspace((int)(c)))
#define ISIDENT(c) (isalnum((int)(c)) || (c) == '_' || (c) == '-')
#define ISVALUE(c) (isprint((int)(c)))

// TODO: needs unit tests
static int config_parse(char *line, const char **sectionp, const char **namep, const char **valuep)
{
  enum state { INIT, START_SECTION, SECTION, NAME, PRE_EQ, POST_EQ, VALUE, END } state = INIT;

  for (char *c = line; *c; c++) {
    switch(state) {
    case INIT:
      if (*c == '\n') {
        return 0;
      } else if (ISSPACE(*c)) {
        continue;
      } else if (*c == '[') {
        state = START_SECTION;
      } else if (ISIDENT(*c)) {
        state = NAME;
        *namep = c;
      } else {
        LOG_WARN("Invalid character at start of line: %c", *c);
        return -1;
      }
      break;

    case START_SECTION:
      if (ISIDENT(*c)) {
        state = SECTION;
        *sectionp = c;
      } else {
        LOG_WARN("Invalid character at start of section: %c", *c);
        return -1;
      }
      break;

    case SECTION:
      if (*c == ']') {
        state = END;
        *c = '\0';
      } else if (ISIDENT(*c)) {
        continue;
      } else {
        LOG_WARN("Invalid character in section: %c", *c);
        return -1;
      }
      break;

    case NAME:
      if (*c == '\n') {
        LOG_WARN("EOL in name");
        return -1;
      } else if (ISSPACE(*c)) {
        *c = '\0';
        state = PRE_EQ;
      } else if (*c == '=') {
        *c = '\0';
        state = POST_EQ;
      } else if (ISIDENT(*c)) {
        continue;
      } else {
        LOG_WARN("Invalid character in name: %c", *c);
        return -1;
      }
      break;

    case PRE_EQ:
      if (*c == '\n') {
        LOG_WARN("EOL before =");
        return -1;
      } else if (ISSPACE(*c)) {
        continue;
      } else if (*c == '=') {
        state = POST_EQ;
      } else {
        LOG_WARN("Invalid character before EQ: %c", *c);
        return -1;
      }
      break;

    case POST_EQ:
      if (*c == '\n') {
        *c = '\0';
        state = END;
        *valuep = c; // empty string
        break;
      } else if (ISSPACE(*c)) {
        continue;
      } else if (ISVALUE(*c)) {
        state = VALUE;
        *valuep = c;
      } else {
        LOG_WARN("Invalid character after EQ: %c", *c);
        return -1;
      }
      break;

    case VALUE:
      if (*c == '\n') {
        *c = '\0';
        state = END;
      } else if (ISVALUE(*c)) {
        continue;
      } else {
        LOG_WARN("Invalid character in value: %c", *c);
        return -1;
      }
      break;

    case END:
      if (ISSPACE(*c)) {
        continue;
      } else {
        LOG_WARN("Invalid character at end: %c", *c);
        return -1;
      }
      break;
    }
  }

  return 0;
}

int config_read(struct config *config, FILE *file)
{
  char buf[CONFIG_LINE];
  int lineno = 0;

  const struct configmod *mod = NULL;
  const struct configtab *tab = NULL;

  while (fgets(buf, sizeof(buf), file) != NULL) {
    const char *section = NULL;
    const char *name = NULL;
    const char *value = NULL;

    lineno++;

    LOG_DEBUG("%s", buf);

    if (config_parse(buf, &section, &name, &value)) {
      LOG_WARN("Invalid line at %s:%d", config->filename, lineno);
      continue;
    }

    if (section) {
      mod = NULL;

      if (configmod_lookup(config->modules, section, &mod)) {
        LOG_WARN("Unknown section: %s", section);
      } else {
        LOG_DEBUG("mod=%s", mod->name);
      }
    }

    if (name && value) {
      if (!mod) {
        LOG_WARN("Invalid name without section: %s", name);
      } else if (configtab_lookup(mod->table, name, &tab)) {
        LOG_WARN("Unknown name in section %s: %s", mod->name, name);
      } else if (config_set(mod, tab, value)) {
        LOG_WARN("Invalid value for section %s name %s: %s", mod->name, tab->name, value);
      } else {
        LOG_DEBUG("mod=%s tab=%s value=%s", mod->name, tab->name, value);
      }
    }
  }

  // TODO
  return 0;
}

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

const struct cmd config_commands[] = {
  { "show",              config_cmd_show, .usage = "[SECTION]",           .describe = "Show config settings"  },
  { "get",               config_cmd_get,  .usage = "SECTION NAME",        .describe = "Get config setting"    },
  { "set",               config_cmd_set,  .usage = "SECTION NAME VALUE",  .describe = "Set and write config"  },
  {}
};

const struct cmdtab config_cmdtab = {
  .commands = config_commands,
  .arg      = &user_configmeta,
};

int init_config(struct config *config)
{
  int err;

  LOG_INFO("Load config=%s", config->filename);

  if ((err = config_load(config))) {
    LOG_WARN("reset config on read error: %d", err);

    return config_save(config);
  }

  return err;
}
