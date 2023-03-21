#pragma once

#include <stddef.h>
#include <stdbool.h>

struct vfs_stat {
  bool mounted;

  size_t sector_size;
  unsigned total_sectors;
  unsigned used_sectors;
  unsigned free_sectors;
};

struct vfs_dev;

typedef int (*vfs_stat_func)(const struct vfs_dev *dev, struct vfs_stat *stat);

struct vfs_dev {
  vfs_stat_func stat_func;
};

struct vfs_mount {
  const char *path;
  const struct vfs_dev *dev;
};

extern const struct vfs_mount vfs_mounts[];

const char *vfs_mount_match(const struct vfs_mount *mount, const char *path);

int vfs_mount_stat(const struct vfs_mount *mount, struct vfs_stat *stat);
