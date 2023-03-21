#pragma once

#include <sdkconfig.h>

#if CONFIG_SDCARD_ENABLED
  #include "vfs_state.h"

  #define SDCARD_VFS_PATH "/sdcard"

  extern const struct vfs_dev sdcard_vfs_dev;
#endif
