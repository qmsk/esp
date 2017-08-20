#include "user_config.h"
#include <fcntl.h>
#include <stdio.h>

int read_config(struct user_config *config)
{
  int fd, ret, err = 0;

  if ((fd = open("config", O_RDONLY)) < 0) {
    printf("ERROR config read: open: %d\n", fd);
    return -1;
  }

  if ((ret = read(fd, config, sizeof(*config))) < 0) {
    err = -1;
    printf("ERROR config read: read: %d\n", ret);
    goto error;
  }

  printf("INFO config read: %d/%d\n", ret, sizeof(*config));

error:
  close(fd);

  return err;
}

int write_config(struct user_config *config)
{
  int fd, ret, err = 0;

  if ((fd = open("config", O_WRONLY | O_CREAT)) < 0) {
    printf("ERROR config write: open: %d\n", fd);
    return -1;
  }

  if ((ret = write(fd, config, sizeof(*config))) < 0) {
    err = -1;
    printf("ERROR config write: write: %d\n", ret);
    goto error;
  }

  printf("INFO config write: %d/%d\n", ret, sizeof(*config));

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
