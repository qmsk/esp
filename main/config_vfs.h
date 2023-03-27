#pragma once

#include "vfs_state.h"

#define CONFIG_VFS_PATH "/config"

int init_config_vfs();
int reset_config_vfs();

extern const struct vfs_dev config_vfs_dev;
