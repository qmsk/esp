#include "spiffs.h"

#include <logging.h>

#include <esp_spiffs.h>
#include <esp_err.h>

int init_spiffs_partition(const char *base_path, const char *partition_label, size_t max_files)
{
  esp_vfs_spiffs_conf_t spiffs_conf = {
    .base_path              = base_path,
    .partition_label        = partition_label,
    .max_files              = max_files,
    .format_if_mount_failed = false,
  };
  esp_err_t err;

  LOG_INFO("mount partition=%s at base_path=%s with max_files=%u", partition_label, base_path, max_files);

  if (!(err = esp_vfs_spiffs_register(&spiffs_conf))) {
    LOG_INFO("partition mounted: %s", spiffs_conf.base_path);
  } else if (err == ESP_ERR_NOT_FOUND) {
    LOG_WARN("partition not found: %s", spiffs_conf.partition_label);
    return 1;
  } else {
    LOG_ERROR("partition esp_vfs_spiffs_register base_path=%s partition_label=%s: %s", base_path, partition_label, esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int init_spiffs_partition_formatted(const char *base_path, const char *partition_label, size_t max_files)
{
  esp_vfs_spiffs_conf_t spiffs_conf = {
    .base_path              = base_path,
    .partition_label        = partition_label,
    .max_files              = max_files,
    .format_if_mount_failed = true,
  };
  esp_err_t err;

  LOG_INFO("mount/format partition=%s at base_path=%s with max_files=%u", partition_label, base_path, max_files);

  if (!(err = esp_vfs_spiffs_register(&spiffs_conf))) {
    LOG_INFO("partition mounted: %s", spiffs_conf.base_path);
  } else if (err == ESP_ERR_NOT_FOUND) {
    LOG_WARN("partition not found: %s", spiffs_conf.partition_label);
    return 1;
  } else {
    LOG_ERROR("partition esp_vfs_spiffs_register base_path=%s partition_label=%s: %s", base_path, partition_label, esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int format_spiffs_partition(const char *partition_label)
{
  esp_err_t err;

  if ((err = esp_spiffs_format(partition_label))) {
    LOG_ERROR("esp_spiffs_format: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int spiffs_info_cmd (int argc, char **argv, void *ctx)
{
  const char *label;
  size_t total_bytes, used_bytes;
  esp_err_t err;

  if (argc >= 2 && (err = cmd_arg_str(argc, argv, 1, &label))) {
    return err;
  }

  if (!(err = esp_spiffs_info(label, &total_bytes, &used_bytes))) {

  } else if (err == ESP_ERR_INVALID_STATE) {
    printf("SPIFFS partition(%s): not mounted\n", label ? label : "");
    return 1;
  } else {
    LOG_ERROR("esp_spiffs_info: %s", esp_err_to_name(err));
  }

  printf("SPIFFS partition(%s): total=%u used=%u (%u%%)\n", label ? label : "", total_bytes, used_bytes, used_bytes * 100 / total_bytes);

  return 0;
}

int spiffs_format_cmd (int argc, char **argv, void *ctx)
{
  const char *label;
  esp_err_t err;

  if (argc >= 2 && (err = cmd_arg_str(argc, argv, 1, &label))) {
    return err;
  }

  if (format_spiffs_partition(label)) {
    LOG_ERROR("format_spiffs_partition");
    return -1;
  } else {
    printf("SPIFFS partition(%s) formatted\n", label ? label : "");
  }

  return 0;
}

const struct cmd spiffs_commands[] = {
  { "info",   spiffs_info_cmd,    .usage = "[LABEL]", .describe = "Show SPIFFS partition"   },
  { "format", spiffs_format_cmd,  .usage = "[LABEL]", .describe = "Format SPIFFS partition" },
  {}
};

const struct cmdtab spiffs_cmdtab = {
  .commands = spiffs_commands,
};
