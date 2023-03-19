#include "config.h"
#include "logging.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

int config_load(struct config *config, const char *filename)
{
  char path[CONFIG_PATH_SIZE];
  FILE *file;
  int err = 0;

  if (snprintf(path, sizeof(path), "%s/%s", config->path, filename) >= sizeof(path)) {
    LOG_ERROR("filename too long: %s", filename);
    return -1;
  }

  if ((file = fopen(path, "r")) == NULL) {
    LOG_ERROR("fopen %s: %s", path, strerror(errno));
    return -1;
  }

  LOG_INFO("%s", path);

  if ((err = config_init(config))) {
    goto error;
  }

  if ((err = config_read(config, file))) {
    goto error;
  }

error:
  fclose(file);

  return err;
}

int config_save(struct config *config, const char *filename)
{
  char newfile[CONFIG_PATH_SIZE];
  char path[CONFIG_PATH_SIZE];
  FILE *file;
  int err = 0;

  if (snprintf(path, sizeof(path), "%s/%s", config->path, filename) >= sizeof(path)) {
    LOG_ERROR("filename too long: %s", filename);
    return -1;
  }

  if (snprintf(newfile, sizeof(newfile), "%s/%s.new", config->path, filename) >= sizeof(newfile)) {
    LOG_ERROR("filename too long: %s.new", filename);
    return -1;
  }

  if ((file = fopen(newfile, "w")) == NULL) {
    LOG_ERROR("fopen %s: %s", newfile, strerror(errno));
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

  if (remove(path)) {
    if (errno == ENOENT) {
      LOG_DEBUG("no existing file");
    } else {
      LOG_ERROR("remove %s: %s", path, strerror(errno));
      return -1;
    }
  }

  if (rename(newfile, path)) {
    LOG_ERROR("rename %s -> %s: %s", newfile, path, strerror(errno));
    return -1;
  }

  return err;
}

int config_delete(struct config *config, const char *filename)
{
  char path[CONFIG_PATH_SIZE];

  if (snprintf(path, sizeof(path), "%s/%s", config->path, filename) >= sizeof(path)) {
    LOG_ERROR("filename too long: %s", filename);
    return -1;
  }

  if (remove(path) == 0) {
    LOG_INFO("remove %s", path);
    return 0;
  } else if (errno == ENOENT) {
    LOG_WARN("remove %s: %s", path, strerror(errno));
    return 1;
  } else {
    LOG_ERROR("remove %s: %s", path, strerror(errno));
    return -1;
  }
}
