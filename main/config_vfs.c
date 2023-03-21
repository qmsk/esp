#include "config_vfs.h"

#include <esp_spiffs.h>
#include <esp_err.h>

#include <logging.h>

#define CONFIG_PARTITON_LABEL "config"
#define CONFIG_MAX_FILES 4

int config_vfs_stat(const struct vfs_dev *dev, struct vfs_stat *stat)
{
  size_t total_size, used_size;
  esp_err_t err;

  if (esp_spiffs_mounted(CONFIG_PARTITON_LABEL)) {
    stat->mounted = true;
  } else {
    stat->mounted = false;
  }

  if ((err = esp_spiffs_info(CONFIG_PARTITON_LABEL, &total_size, &used_size))) {
    if (err == ESP_ERR_INVALID_STATE) {
      LOG_DEBUG("esp_spiffs_info: %s", esp_err_to_name(err));
    } else {
      LOG_ERROR("esp_spiffs_info: %s", esp_err_to_name(err));
      return -1;
    }
  } else {
    stat->sector_size = 1;
    stat->total_sectors = total_size;
    stat->used_sectors = used_size;
    stat->free_sectors = total_size - used_size;
  }

  return 0;
}

const struct vfs_dev config_vfs_dev = {
  .stat_func = config_vfs_stat,
};

int init_config_vfs()
{
  esp_vfs_spiffs_conf_t conf = {
    .base_path              = CONFIG_VFS_PATH,
    .partition_label        = CONFIG_PARTITON_LABEL,
    .max_files              = CONFIG_MAX_FILES,
    .format_if_mount_failed = true,
  };
  esp_err_t err;

  LOG_INFO("mount/format partition=%s at base_path=%s with max_files=%u", conf.partition_label, conf.base_path, conf.max_files);

  if ((err = esp_vfs_spiffs_register(&conf))) {
    if (err == ESP_ERR_NOT_FOUND) {
      LOG_WARN("config partition with label=%s not found", conf.partition_label);
      return 1;
    } else {
      LOG_ERROR("esp_vfs_spiffs_register: %s", esp_err_to_name(err));
      return -1;
    }
  }

  return 0;
}

int reset_config_vfs()
{
  esp_err_t err;

  LOG_WARN("format config filesystem with partition label=%s", CONFIG_PARTITON_LABEL);

  if ((err = esp_spiffs_format(CONFIG_PARTITON_LABEL))) {
    LOG_ERROR("esp_spiffs_format: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}
