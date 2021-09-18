#include "config.h"
#include "logging.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

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
    if (errno == ENOENT) {
      LOG_DEBUG("no existing file");
    } else {
      LOG_ERROR("remove %s: %s", config->filename, strerror(errno));
      return -1;
    }
  }

  if (rename(newfile, config->filename)) {
    LOG_ERROR("rename %s -> %s: %s", newfile, config->filename, strerror(errno));
    return -1;
  }

  return err;
}

int config_reset(struct config *config)
{
  if (remove(config->filename) == 0) {
    LOG_INFO("remove %s", config->filename);
    return 0;
  } else if (errno == ENOENT) {
    LOG_WARN("remove %s: %s", config->filename, strerror(errno));
    return 1;
  } else {
    LOG_ERROR("remove %s: %s", config->filename, strerror(errno));
    return -1;
  }
}
