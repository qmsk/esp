#include "spiffs.h"

#include <logging.h>

#include <esp_spiffs.h>
#include <esp_err.h>

#define SPIFFS_CONFIG_MAX_FILES 5

int init_spiffs()
{
  esp_vfs_spiffs_conf_t config_spiffs = {
    .base_path              = "/config",
    .partition_label        = "config",
    .max_files              = SPIFFS_CONFIG_MAX_FILES,
    .format_if_mount_failed = true,
  };
  esp_err_t err;

  if (!(err = esp_vfs_spiffs_register(&config_spiffs))) {
    LOG_INFO("config partition mounted: %s", config_spiffs.base_path);
  } else if (err == ESP_ERR_NOT_FOUND) {
    LOG_WARN("config partition not found");
  } else {
    LOG_ERROR("config partition esp_vfs_spiffs_register: %s", esp_err_to_name(err));
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

  if ((err = esp_spiffs_format(label))) {
    LOG_ERROR("esp_spiffs_format: %s", esp_err_to_name(err));
  }

  printf("SPIFFS partition(%s) formatted\n", label ? label : "");

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
