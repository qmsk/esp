#include "config.h"
#include "logging.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/dirent.h>

static bool match_fileext(const char *filename, const char *ext)
{
  const char *suffix = strrchr(filename, '.');

  if (!suffix) {
    return false;
  }

  if (strcmp(suffix + 1, ext)) {
    return false;
  }

  return true;
}

static int config_path(struct config *config, const char *filename, const char *ext, char *buf, size_t size)
{
  if (!match_fileext(filename, ext)) {
    LOG_ERROR("filename must end with .%s: %s", ext, filename);
    return -1;
  }

  if (snprintf(buf, size, "%s/%s", config->path, filename) >= size) {
    LOG_ERROR("filename too long: %s", filename);
    return -1;
  }

  return 0;
}

int config_walk(struct config *config, int (func)(const char *filename, void *ctx), void *ctx)
{
  DIR *dir;
  struct dirent *d;
  int err = 0;

  if (!(dir = opendir(config->path))) {
    LOG_ERROR("opendir %s: %s", config->path, strerror(errno));
    return -1;
  }

  while ((d = readdir(dir))) {
    const char *filename = d->d_name;

    if (!match_fileext(filename, CONFIG_FILE_EXT)) {
      continue;
    }

    if ((err = func(filename, ctx))) {
      break;
    }
  }

  closedir(dir);

  return err;
}

int config_load(struct config *config, const char *filename)
{
  char path[CONFIG_PATH_SIZE];
  FILE *file;
  int err = 0;

  if ((err = config_path(config, filename, CONFIG_FILE_EXT, path, sizeof(path)))) {
    return err;
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
  char path[CONFIG_PATH_SIZE];
  char newfile[CONFIG_PATH_SIZE];
  FILE *file;
  int err = 0;

  if ((err = config_path(config, filename, CONFIG_FILE_EXT, path, sizeof(path)))) {
    return err;
  }

  if (snprintf(newfile, sizeof(newfile), "%s.new", path) >= sizeof(newfile)) {
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
  int err;

  if ((err = config_path(config, filename, CONFIG_FILE_EXT, path, sizeof(path)))) {
    return err;
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
