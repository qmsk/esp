#include "config_vfs.h"
#include "vfs_state.h"

#include <sdkconfig.h>

const struct vfs_mount vfs_mounts[] = {
  { .path = CONFIG_VFS_PATH, .dev = &config_vfs_dev },
#if CONFIG_SDCARD_ENABLED
  { .path = "/sdcard" },
#endif
  { },
};

const char *vfs_mount_match(const struct vfs_mount *mount, const char *path)
{
  const char *p = path, *m = mount->path;

  // scan common prefix
  while (*p && *p++ == *m++ && *m) {

  }

  if (!*p) {
    if (!*m) {
      // exact match
      return p;
    } else {
      // short
      return NULL;
    }
  } else if (!*m) {
    if (*p == '/') {
      // directory prefix match
      return p + 1;
    } else {
      return false;
    }
  } else {
    return false;
  }
}
