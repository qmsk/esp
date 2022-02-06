#include "vfs.h"

#include <logging.h>

#include <esp_err.h>
#include <esp_vfs.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/dirent.h>

static char dirent_type_char(const struct dirent *d) {
  if (d->d_type == DT_DIR) {
    return 'd';
  } else if (d->d_type == DT_REG) {
    return 'f';
  } else {
    return '?';
  }
}

esp_err_t vfs_walk_func(esp_vfs_id_t id, const char *path, void *ctx)
{
  printf("%c <%d> %s\n", 'v', id, path);

  return 0;
}

int vfs_ls_root()
{
  esp_err_t err;

  if ((err = esp_vfs_walk_paths(vfs_walk_func, NULL))) {
    LOG_ERROR("esp_vfs_walk_paths: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int vfs_ls_path(const char *path)
{
  DIR *dir;
  struct dirent *d;

  if (!(dir = opendir(path))) {
    LOG_ERROR("opendir %s: %s", path, strerror(errno));
    return -1;
  }

  // ignore any errors returned by readdir()
  // https://github.com/pellepl/spiffs/issues/64#issuecomment-171570238 returns an undocumented internal error code in normal case
  while ((d = readdir(dir))) {
    printf("%c %s\n", dirent_type_char(d), d->d_name);
  }

  closedir(dir);

  return 0;
}

int vfs_ls_cmd(int argc, char **argv, void *ctx)
{
  const char *path = NULL;
  int err = 0;

  if (argc >= 2 && (err = cmd_arg_str(argc, argv, 1, &path))) {
    return err;
  }

  if (!path || strlen(path) == 0 || strcmp(path, "/") == 0) {
    return vfs_ls_root();
  } else {
    return vfs_ls_path(path);
  }

  return err;
}

esp_err_t vfs_walk_fd_func(esp_vfs_id_t id, int fd, void *ctx)
{
  printf("%3d <%d>\n", fd, id);

  return 0;
}

int vfs_lsof_cmd(int argc, char **argv, void *ctx)
{
  esp_err_t err;

  if ((err = esp_vfs_walk_fds(vfs_walk_fd_func, NULL))) {
    LOG_ERROR("esp_vfs_walk_fds: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

const struct cmd vfs_commands[] = {
  { "ls",     vfs_ls_cmd,    .usage = "[PATH]", .describe = "List files"  },
  { "lsof",   vfs_lsof_cmd,                     .describe = "List open file descriptors"  },
  {}
};

const struct cmdtab vfs_cmdtab = {
  .commands = vfs_commands,
};
