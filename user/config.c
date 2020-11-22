#include "config.h"
#include "user_config.h"
#include "user_cmd.h"
#include "cli.h"
#include "logging.h"

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
int config_lookup(const struct config_tab *tab, const char *name, const struct config_tab **tabp)
{
  for (; tab->type && tab->name; tab++) {
    if (strcmp(tab->name, name) == 0) {
      *tabp = tab;
      return 0;
    }
  }

  return 1;
}

int config_set(const struct config_tab *tab, const char *value)
{
  unsigned uvalue;

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

int config_print(const struct config_tab *tab)
{
  switch(tab->type) {
    case CONFIG_TYPE_NULL:
      break;

    case CONFIG_TYPE_STRING:
      cli_printf("%s = %s\n", tab->name, tab->value.string);
      break;

    case CONFIG_TYPE_UINT16:
      cli_printf("%s = %u\n", tab->name, *tab->value.uint16);
      break;

    default:
      return -CMD_ERR_NOT_IMPLEMENTED;
  }

  return 0;
}

int config_cmd_list(int argc, char **argv, void *ctx)
{
  const struct config_tab *configtab = ctx;

  for (const struct config_tab *tab = configtab; tab->type && tab->name; tab++) {
    config_print(tab);
  }

  return 0;
}

int config_cmd_set(int argc, char **argv, void *ctx)
{
  const struct config_tab *configtab = ctx, *tab;
  const char *name, *value;

  int err;

  if ((err = cmd_arg_str(argc, argv, 1, &name))) {
    return err;
  }
  if ((err = cmd_arg_str(argc, argv, 2, &value))) {
    return err;
  }

  if (config_lookup(configtab, name, &tab)) {
    LOG_ERROR("Unkown configtab: %s", name);
    return -CMD_ERR_ARGV;
  }

  if (config_set(tab, value)) {
    LOG_ERROR("Invalid configtab %s value: %s", tab->name, value);
    return -CMD_ERR_ARGV;
  }

  if (config_write(&user_config)) {
    LOG_ERROR("Failed writing config");
    return -CMD_ERR;
  }

  return 0;
}

const struct cmd config_commands[] = {
  { "list",              config_cmd_list, (void *) user_configtab,   .describe = "List config settings" },
  { "set",               config_cmd_set,  (void *) user_configtab,   .usage = "NAME VALUE", .describe = "Set and write config" },
  {}
};

const struct cmdtab config_cmdtab = {
  .commands = config_commands,
};
