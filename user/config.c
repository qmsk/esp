#include "user_config.h"
#include "logging.h"

#include <fcntl.h>
#include <stdio.h>

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

  LOG_INFO("read: %d/%d", ret, sizeof(*config));

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

int init_config(struct user_config *config)
{
  int err;

  if ((err = read_config(config)) == 0) {
    return 0;
  } else if ((err = write_config(config)) == 0) {
    return 0;
  } else {
    return err;
  }
}
