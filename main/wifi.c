#include "wifi.h"
#include "wifi_internal.h"

#include <logging.h>
#include <system_wifi.h>

#include <esp_err.h>
#include <esp_netif.h>
#include <esp_wifi.h>

esp_netif_t *wifi_ap_netif, *wifi_sta_netif;

/*
 * Switch from NULL -> AP/STA -> AP_STA mode as required.
 */
static int switch_wifi_mode(wifi_mode_t to_mode)
{
  wifi_mode_t from_mode;
  esp_err_t err;

  if ((err = esp_wifi_get_mode(&from_mode))) {
    LOG_ERROR("esp_wifi_get_mode: %s", esp_err_to_name(err));
    return -1;
  }

  if (from_mode != WIFI_MODE_NULL && from_mode != to_mode && to_mode != WIFI_MODE_NULL) {
    // hybrid
    to_mode = WIFI_MODE_APSTA;
  }

  if (from_mode != to_mode) {
    LOG_INFO("switch from mode=%s to mode=%s", wifi_mode_str(from_mode), wifi_mode_str(to_mode));

    if ((err = esp_wifi_set_mode(to_mode))) {
      LOG_ERROR("esp_wifi_set_mode %s: %s", wifi_mode_str(to_mode), esp_err_to_name(err));
      return -1;
    }
  }

  return 0;
}

int start_wifi()
{
  esp_err_t err;

  LOG_INFO("starting...");

  if ((err = esp_wifi_start())) {
    LOG_ERROR("esp_wifi_start: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int wifi_scan(const wifi_scan_config_t *scan_config, int (*cb)(wifi_ap_record_t *ap, void *ctx), void *ctx)
{
  wifi_ap_record_t *wifi_ap_records;
  uint16_t ap_num;
  int err;

  // cannot scan in AP mode
  if ((err = switch_wifi_mode(WIFI_MODE_STA))) {
    LOG_ERROR("switch_wifi_mode WIFI_MODE_STA");
    return err;
  }
  if ((err = esp_wifi_start())) {
    LOG_ERROR("esp_wifi_start: %s", esp_err_to_name(err));
    return err;
  }

  LOG_INFO("scan: start ssid=%s",
    scan_config->ssid ? (const char *) scan_config->ssid : "*"
  );

  if ((err = esp_wifi_scan_start(scan_config, true))) {
    LOG_ERROR("esp_wifi_scan_start: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_scan_get_ap_num(&ap_num))) {
    LOG_ERROR("esp_wifi_scan_get_ap_num: %s", esp_err_to_name(err));
    return -1;
  }

  LOG_INFO("scan: get ap_num=%u APs", ap_num);

  if (!(wifi_ap_records = calloc(ap_num, sizeof(*wifi_ap_records)))) {
    LOG_ERROR("calloc(%d)", ap_num);
    return -1;
  }

  if ((err = esp_wifi_scan_get_ap_records(&ap_num, wifi_ap_records))) {
    LOG_ERROR("esp_wifi_scan_get_ap_records: %s", esp_err_to_name(err));
    goto error;
  }

  for (int i = 0; i < ap_num; i++) {
    if ((err = cb(&wifi_ap_records[i], ctx))) {
      LOG_WARN("callback ret=%d", err);
      goto error;
    }
  }

error:
  free(wifi_ap_records);

  return err;
}

int wifi_listen(const wifi_ap_config_t *ap_config)
{
  wifi_config_t config = { .ap = *ap_config };
  esp_err_t err;

  // mode
  if ((err = switch_wifi_mode(WIFI_MODE_AP))) {
    LOG_ERROR("switch_wifi_mode WIFI_MODE_AP");
    return err;
  }

  // netif
  if (wifi_ap_netif) {

  } else if (!(wifi_ap_netif = esp_netif_create_default_wifi_ap())) {
    LOG_ERROR("esp_netif_create_default_wifi_ap");
    return -1;
  } else {
    LOG_INFO("created wifi AP netif=%p", wifi_ap_netif);
  }

  // config
  LOG_INFO("channel=%u ssid=%.32s password=%s authmode=%s ssid_hidden=%s max_connection=%d",
    config.ap.channel,
    config.ap.ssid,
    config.ap.password[0] ? "***" : "",
    wifi_auth_mode_str(config.ap.authmode),
    config.ap.ssid_hidden ? "true" : "false",
    config.ap.max_connection
  );

  if ((err = esp_wifi_set_config(ESP_IF_WIFI_AP, &config))) {
    LOG_ERROR("esp_wifi_set_config ESP_IF_WIFI_AP: %s", esp_err_to_name(err));
    return -1;
  }

  // start
  if ((err = esp_wifi_start())) {
    LOG_ERROR("esp_wifi_start: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int stop_wifi()
{
  esp_err_t err;

  LOG_INFO("stopping...");

  if ((err = esp_wifi_stop())) {
    LOG_ERROR("esp_wifi_stop: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int config_wifi()
{
  esp_err_t err;

  // TODO
  if ((err = esp_wifi_set_mode(WIFI_MODE_NULL))) {
    LOG_ERROR("esp_wifi_set_mode: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int init_wifi()
{
  wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
  esp_err_t err;

  if ((err = init_wifi_events())) {
    LOG_ERROR("init_wifi_events");
    return err;
  }

  if ((err = esp_wifi_init(&wifi_config))) {
    LOG_ERROR("esp_wifi_init: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_set_storage(WIFI_STORAGE_RAM))) {
    LOG_ERROR("esp_wifi_set_storage: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = config_wifi())) {
    LOG_ERROR("config_wifi");
    return err;
  }

  return 0;
}
