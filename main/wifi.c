#include "wifi.h"
#include "user_event.h"

#include <logging.h>
#include <system_wifi.h>

#include <esp_err.h>
#include <esp_wifi.h>
#include <tcpip_adapter.h>

/*
 * Switch state if needed for scanning.
 */
int wifi_scan_mode()
{
  wifi_mode_t from_mode, to_mode;
  wifi_state_t state = esp_wifi_get_state();
  esp_err_t err;

  LOG_DEBUG("state=%d", state);

  if ((err = esp_wifi_get_mode(&from_mode))) {
    LOG_ERROR("esp_wifi_get_mode: %s", esp_err_to_name(err));
    return -1;
  }

  switch (from_mode) {
    case WIFI_MODE_NULL:
      to_mode = WIFI_MODE_STA;
      break;

    case WIFI_MODE_STA:
      return 0;

    case WIFI_MODE_AP:
      to_mode = WIFI_MODE_APSTA;
      break;

    case WIFI_MODE_APSTA:
      return 0;

    default:
      LOG_ERROR("unknown mode=%d", from_mode);
      return -1;
  }

  LOG_INFO("switch from state=%d / mode=%s to mode=%s", state, wifi_mode_str(from_mode), wifi_mode_str(to_mode));

  if ((err = esp_wifi_set_mode(to_mode))) {
    LOG_ERROR("esp_wifi_set_mode %s: %s", wifi_mode_str(to_mode), esp_err_to_name(err));
    return -1;
  }

  if (state == WIFI_STATE_START) {

  } else if ((err = esp_wifi_start())) {
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

  if ((err = wifi_scan_mode())) {
    LOG_ERROR("wifi_scan_mode");
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

int wifi_connect(wifi_mode_t mode, const wifi_sta_config_t *sta_config)
{
  wifi_config_t config = { .sta = *sta_config };
  esp_err_t err;

  if ((err = esp_wifi_set_mode(mode))) {
    LOG_ERROR("esp_wifi_set_mode %s: %s", wifi_mode_str(mode), esp_err_to_name(err));
    return -1;
  }

  LOG_INFO("channel=%u ssid=%.32s password=%s authmode=%s",
    config.sta.channel,
    config.sta.ssid,
    config.sta.password[0] ? "***" : "",
    wifi_auth_mode_str(config.sta.threshold.authmode)
  );

  if ((err = esp_wifi_set_config(ESP_IF_WIFI_STA, &config))) {
    LOG_ERROR("esp_wifi_set_config ESP_IF_WIFI_STA: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_connect())) {
    LOG_ERROR("esp_wifi_connect: %s", esp_err_to_name(err));
    return -1;
  }

  user_state(USER_STATE_CONNECTING);

  return 0;
}

int wifi_listen(wifi_mode_t mode, const wifi_ap_config_t *ap_config)
{
  wifi_config_t config = { .ap = *ap_config };
  esp_err_t err;

  if ((err = esp_wifi_set_mode(mode))) {
    LOG_ERROR("esp_wifi_set_mode %s: %s", wifi_mode_str(mode), esp_err_to_name(err));
    return -1;
  }

  LOG_INFO("channel=%u ssid=%.32s password=%s authmode=%s ssid_hidden=%s max_connection=%d",
    config.ap.channel,
    config.ap.ssid,
    config.ap.password[0] ? "***" : "",
    wifi_auth_mode_str(config.ap.authmode),
    config.ap.ssid_hidden ? "true" : "false",
    config.ap.max_connection
  );

  user_state(USER_STATE_CONNECTING);

  if ((err = esp_wifi_set_config(ESP_IF_WIFI_AP, &config))) {
    LOG_ERROR("esp_wifi_set_config ESP_IF_WIFI_AP: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int wifi_close()
{
  esp_err_t err;

  if ((err = esp_wifi_set_mode(WIFI_MODE_NULL))) {
    LOG_ERROR("esp_wifi_set_mode: %s", esp_err_to_name(err));
    return -1;
  }

  // most likely also set via wifi_events
  user_state(USER_STATE_DISCONNECTED);

  return 0;
}
