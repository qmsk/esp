#include "config.h"
#include "user_config.h"
#include "cli.h"
#include "logging.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

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

int config_cmd_show(int argc, char **argv, void *ctx)
{
  struct user_config *config = ctx;

  cli_printf("version=%u\n", config->version);

  return 0;
}

const struct cmd config_commands[] = {
  { "show", config_cmd_show, .describe = "Show configuration values" },
  {}
};
