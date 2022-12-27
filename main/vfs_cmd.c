#include "vfs.h"

#include <logging.h>

#include <esp_err.h>
#include <esp_vfs.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/dirent.h>

#include <sdkconfig.h>

static char dirent_type_char(const struct dirent *d) {
  if (d->d_type == DT_DIR) {
    return 'd';
  } else if (d->d_type == DT_REG) {
    return 'f';
  } else {
    return '?';
  }
}

static char stat_mode_char(const struct stat *st) {
  switch (st->st_mode & _IFMT) {
    case _IFDIR:
      return 'd';

    case _IFREG:
      return 'f';

    default:
      return '?';
  }
}

#if !CONFIG_IDF_TARGET_ESP8266

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

#endif

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
  #if CONFIG_IDF_TARGET_ESP8266
    return CMD_ERR_ARGV;
  #else
    return vfs_ls_root();
  #endif
  } else {
    return vfs_ls_path(path);
  }

  return err;
}

#if !CONFIG_IDF_TARGET_ESP8266

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

#endif

int vfs_stat_cmd(int argc, char **argv, void *ctx)
{
  struct stat st;
  struct tm tm;
  char tm_buf[20];
  const char *path = NULL;
  int err = 0;

  if ((err = cmd_arg_str(argc, argv, 1, &path))) {
    return err;
  }

  if (stat(path, &st)) {
    LOG_ERROR("stat %s: %s", path, strerror(errno));
    return -1;
  }

  localtime_r(&st.st_mtime, &tm);
  strftime(tm_buf, sizeof(tm_buf), "%F %T", &tm);

  printf("%c %5ld %20s %s\n", stat_mode_char(&st), st.st_size, tm_buf, path);

  return 0;
}

int vfs_mv_cmd(int argc, char **argv, void *ctx)
{
  const char *src_path = NULL;
  const char *dst_path = NULL;
  int err = 0;

  if ((err = cmd_arg_str(argc, argv, 1, &src_path))) {
    return err;
  }

  if ((err = cmd_arg_str(argc, argv, 2, &dst_path))) {
    return err;
  }

  if (rename(src_path, dst_path)) {
    LOG_ERROR("rename %s -> %s: %s", src_path, dst_path, strerror(errno));
    return -1;
  }

  return 0;
}

int vfs_rm_cmd(int argc, char **argv, void *ctx)
{
  const char *path = NULL;
  int err = 0;

  if ((err = cmd_arg_str(argc, argv, 1, &path))) {
    return err;
  }

  if (unlink(path)) {
    LOG_ERROR("unlink %s: %s", path, strerror(errno));
    return -1;
  }

  return 0;
}

const struct cmd vfs_commands[] = {
  { "ls",     vfs_ls_cmd,    .usage = "[PATH]", .describe = "List files"  },
#if !CONFIG_IDF_TARGET_ESP8266
  { "lsof",   vfs_lsof_cmd,                     .describe = "List open file descriptors"  },
#endif
  { "stat",   vfs_stat_cmd,   .usage = "PATH",    .describe = "Stat file"  },
  { "mv",     vfs_mv_cmd,     .usage = "SRC DST", .describe = "Rename file"  },
  { "rm",     vfs_rm_cmd,     .usage = "PATH",    .describe = "Remove file"  },
  {}
};

const struct cmdtab vfs_cmdtab = {
  .commands = vfs_commands,
};
