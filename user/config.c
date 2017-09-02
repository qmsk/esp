#include "config.h"
#include "user_config.h"
#include "user_cmd.h"
#include "cli.h"
#include "logging.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

struct user_config user_config = {
  .version        = USER_CONFIG_VERSION,
  .wifi_ssid      = USER_CONFIG_WIFI_SSID,
  .wifi_password  = USER_CONFIG_WIFI_PASSWORD,
  .artnet = {
    .universe = ARTNET_CONFIG_UNIVERSE,
  },
  .dmx = {
    .gpio = DMX_CONFIG_GPIO,
    .artnet_universe = DMX_CONFIG_ARTNET_UNIVERSE,
  },
};

int read_config(struct user_config *config)
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

int write_config(struct user_config *config)
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

int upgrade_config(struct user_config *config, const struct user_config *upgrade_config)
{
  LOG_INFO("invalid config version=%d: expected version %d", upgrade_config->version, config->version);

  // TODO: upgrade/downgrade?
  return write_config(config);
}

int load_config(struct user_config *config, const struct user_config *load_config)
{
  *config = *load_config;

  LOG_INFO("version=%d", config->version);

  return 0;
}

int init_config(struct user_config *config)
{
  struct user_config stored_config;
  int err;

  if ((err = read_config(&stored_config))) {
    LOG_WARN("reset config on read error: %d", err);

    return write_config(config);
  } else if (stored_config.version != config->version) {
    return upgrade_config(config, &stored_config);
  } else {
    return load_config(config, &stored_config);
  }
}


static struct config_tab config_version = { CONFIG_TYPE_UINT16, "version",
  .readonly = true,
  .value = { .uint16 = &user_config.version },
};
static struct config_tab config_wifi_ssid = { CONFIG_TYPE_STRING, "wifi.ssid",
  .size = sizeof(user_config.wifi_ssid),
  .value = { .string = user_config.wifi_ssid },
};
static struct config_tab config_wifi_password = { CONFIG_TYPE_STRING, "wifi.password",
  .size = sizeof(user_config.wifi_password),
  .value = { .string = user_config.wifi_password },
};
static struct config_tab config_artnet_universe = { CONFIG_TYPE_UINT16, "artnet.universe",
  .size = sizeof(user_config.artnet.universe),
  .value = { .uint16 = &user_config.artnet.universe },
};
static struct config_tab config_dmx_gpio = { CONFIG_TYPE_UINT16, "dmx.gpio",
  .value = { .uint16 = &user_config.dmx.gpio },
};
static struct config_tab config_dmx_artnet_universe = { CONFIG_TYPE_UINT16, "dmx.artnet-universe",
  .value = { .uint16 = &user_config.dmx.artnet_universe },
};

int config_set(const struct config_tab *tab, const char *value)
{
  unsigned uvalue;

  switch (tab->type) {
    case CONFIG_TYPE_STRING:
      if (snprintf(tab->value.string, tab->size, "%s", value) >= tab->size) {
        return -CMD_ERR_ARGV;
      } else {
        break;
      }

    case CONFIG_TYPE_UINT16:
      if (sscanf(value, "%u", &uvalue) <= 0) {
        return -CMD_ERR_ARGV;
      } else if (uvalue > UINT16_MAX) {
        return -CMD_ERR_ARGV;
      } else {
        *tab->value.uint16 = (uint16_t) uvalue;
        break;
      }

    default:
      return -CMD_ERR_ARGV;
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

int config_cmd(int argc, char **argv, void *ctx)
{
  struct config_tab *tab = ctx;
  char *value;
  int err;

  if (argc == 1) {
    value = NULL;
  } else if (argc == 2) {
    value = argv[1];
  } else {
    return -CMD_ERR_ARGC;
  }

  if (!value) {

  } else if (tab->readonly) {
    return -CMD_ERR_ARGC;
  } else if ((err = config_set(tab, value))) {
    return err;
  }

  return config_print(tab);

  return 0;
}

const struct cmd config_commands[] = {
  { "version",          config_cmd, &config_version,          .describe = "Version" },
  { "wifi.ssid",        config_cmd, &config_wifi_ssid,        .usage = "[SSID]", .describe = "WiFi SSID"     },
  { "wifi.password",    config_cmd, &config_wifi_password,    .usage = "[PASSWORD]", .describe = "WiFi Password" },
  { "artnet.universe",         config_cmd, &config_artnet_universe,     .usage = "[UNIVERSE-BASE]", .describe = "ArtNet output port base address" },
  { "dmx.artnet-universe",     config_cmd, &config_dmx_artnet_universe, .usage = "[UNIVERSE]", .describe = "ArtNet DMX output port address" },
  { "dmx.gpio",                config_cmd, &config_dmx_gpio,            .usage = "[GPIO]", .describe = "DMX Output Enable GPIO pin (low during boot)" },
  {}
};

const struct cmdtab config_cmdtab = {
  .commands = config_commands,
};
