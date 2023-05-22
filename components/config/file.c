#include "config.h"
#include "logging.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/dirent.h>
#include <sys/stat.h>

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

int config_file_path(const struct config_file_path *paths, const char *value, char *buf, size_t size)
{
  const struct config_file_path *p = paths;
  struct stat st;

  for (; p->prefix; p++) {
    if (!match_fileext(value, p->suffix)) {
      continue;
    }

    if (snprintf(buf, size, "%s/%s", p->prefix, value) >= size) {
      LOG_WARN("overflow");
      return -1;
    }

    if (stat(buf, &st)) {
      if (errno == ENOENT) {
        continue;
      } else {
        LOG_ERROR("stat %s: %s", buf, strerror(errno));
        return -1;
      }
    }

    return 0;
  }

  return 1;
}

int config_file_check(const struct config_file_path *paths, const char *value)
{
  char path[CONFIG_PATH_SIZE];
  int err;

  if ((err = config_file_path(paths, value, path, sizeof(path)))) {
    return err;
  }

  return 0;
}

int config_file_walk(const struct config_file_path *paths, int (*func)(const struct config_file_path *path, const char *name, void *ctx), void *ctx)
{
  DIR *dir;
  struct dirent *d;
  int ret = 0;

  for (const struct config_file_path *p = paths; p->prefix; p++) {
    if (!(dir = opendir(p->prefix))) {
      if (errno == ENOENT) {
        LOG_DEBUG("opendir %s: %s", p->prefix, strerror(errno));
        continue;
      } else {
        LOG_ERROR("opendir %s: %s", p->prefix, strerror(errno));
        return -1;
      }
    }

    while ((d = readdir(dir))) {
      if (d->d_type != DT_REG) {
        continue;
      }

      if (!match_fileext(d->d_name, p->suffix)) {
        continue;
      }

      if ((ret = func(p, d->d_name, ctx))) {
        break;
      }
    }

    closedir(dir);

    if (ret) {
      break;
    }
  }

  return ret;
}

FILE *config_file_open(const struct config_file_path *paths, const char *value)
{
  char path[PATH_MAX];
  FILE *file = NULL;
  int err;

  if ((err = config_file_path(paths, value, path, sizeof(path)))) {
    return NULL;
  }

  if (!(file = fopen(path, "r"))) {
    LOG_ERROR("fopen %s: %s", path, strerror(errno));
    return NULL;
  } else {
    LOG_INFO("%s: %s", value, path);
  }

  return file;
}
