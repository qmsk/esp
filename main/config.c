#include "config.h"
#include "artnet.h"
#include "http.h"
#include "wifi.h"

#include <config.h>
#include <config_cmd.h>
#include <logging.h>

#include <esp_err.h>
#include <esp_spiffs.h>

#include <errno.h>

#define CONFIG_BASE_PATH "/config"
#define CONFIG_FILE_NAME "boot.ini"
#define CONFIG_PARTITON_LABEL "config"
#define CONFIG_MAX_FILES 4

const struct configmod config_modules[] = {
  { "wifi",
    .description = (
      "WiFi AP/STA mode, using static/dynamic IPv4 addressing."
    ),
    .table = wifi_configtab,
  },
  { "http",
    .description = "HTTP API + Web frontend with optional HTTP basic authentication.",
    .table = http_configtab,
  },
  { "artnet",
    .description = (
      "Art-Net receiver on UDP port 6454."
      "\n"
      "Art-Net addresses consist of the net (0-127) + subnet (0-15) + universe (0-15)."
      " All outputs share the same net/subnet, each output uses a different universe."
      " Up to four outputs are supported."
    ),
    .table = artnet_configtab,
  },
  {}
};

static bool config_disabled = false;
struct config config = {
  .filename = (CONFIG_BASE_PATH "/" CONFIG_FILE_NAME),

  .modules = config_modules,
};

void disable_config()
{
  LOG_WARN("Disabling config loading");
  config_disabled = true;
}

int init_config()
{
  esp_vfs_spiffs_conf_t conf = {
    .base_path              = CONFIG_BASE_PATH,
    .partition_label        = CONFIG_PARTITON_LABEL,
    .max_files              = CONFIG_MAX_FILES,
    .format_if_mount_failed = true,
  };
  esp_err_t err;

  LOG_INFO("mount/format partition=%s at base_path=%s with max_files=%u", conf.partition_label, conf.base_path, conf.max_files);

  if ((err = esp_vfs_spiffs_register(&conf))) {
    if (err == ESP_ERR_NOT_FOUND) {
      LOG_WARN("config partition with label=%s not found", conf.partition_label);
    } else {
      LOG_ERROR("esp_vfs_spiffs_register: %s", esp_err_to_name(err));
    }
  }

  if (config_disabled) {
    LOG_WARN("Configuration loading disabled");
    return 1;
  }

  if (config_load(&config)) {
    if (errno == ENOENT) {
      LOG_WARN("spiffs file at %s not found", config.filename);
      return 1;
    } else {
      LOG_ERROR("config_load(%s)", config.filename);
      return -1;
    }
  }

  LOG_INFO("loaded config");

  return 0;
}

void reset_config()
{
  int err;

  // attempt to remove the config boot.ini file
  if ((err = config_reset(&config)) < 0) {
    LOG_ERROR("config_reset");
  } else if (err > 0) {
    LOG_INFO("no config file: %s", config.filename);
    return;
  } else {
    LOG_WARN("removed config file: %s", config.filename);
    return; // success
  }

  // if spiffs is corrupted enough, format entire partition
  if ((err = esp_spiffs_format(CONFIG_PARTITON_LABEL))) {
    LOG_ERROR("esp_spiffs_format: %s", esp_err_to_name(err));
  } else {
    LOG_WARN("formatted config filesystem with partition label=%s", CONFIG_PARTITON_LABEL);
    return; // success
  }

  LOG_WARN("unable to reset configuration!");
}
