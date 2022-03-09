#include "wifi.h"
#include "wifi_state.h"
#include "wifi_interface.h"
#include "wifi_config.h"
#include "user.h"

#include <logging.h>
#include <system_wifi.h>

#include <esp_err.h>
#include <esp_wifi.h>

bool wifi_sta_connect, wifi_sta_started, wifi_sta_connected;
bool wifi_ap_listen, wifi_ap_started;
unsigned wifi_ap_connected;

int init_wifi()
{
  wifi_init_config_t wifi_ini_config = WIFI_INIT_CONFIG_DEFAULT();
  esp_err_t err;

  if ((err = esp_wifi_init(&wifi_ini_config))) {
    LOG_ERROR("esp_wifi_init: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_set_storage(WIFI_STORAGE_RAM))) {
    LOG_ERROR("esp_wifi_set_storage: %s", esp_err_to_name(err));
    return -1;
  }

  // add user handlers *after* esp_wifi_init() -> tcpip_adapter_set_default_wifi_handlers()
  // we want our event handlers to run after the the tcpip_adapter has been started 
  if ((err = init_wifi_events())) {
    LOG_ERROR("init_wifi_events");
    return err;
  }

  if ((err = config_wifi(&wifi_config))) {
    LOG_ERROR("config_wifi");
    return err;
  }

  return 0;
}

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
  if ((err = init_wifi_interface(WIFI_IF_AP))) {
    LOG_ERROR("init_wifi_interface");
    return err;
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

    // state
  wifi_ap_listen = true;

  return 0;
}

int wifi_connect(const wifi_sta_config_t *sta_config)
{
  wifi_config_t config = { .sta = *sta_config };
  esp_err_t err;

  // mode
  if ((err = switch_wifi_mode(WIFI_MODE_STA))) {
    LOG_ERROR("switch_wifi_mode WIFI_MODE_STA");
    return err;
  }

  // netif
  if ((err = init_wifi_interface(WIFI_IF_STA))) {
    LOG_ERROR("init_wifi_interface");
    return err;
  }

  // config
  LOG_INFO("channel=%u ssid=%.32s password=%s authmode=%s",
    config.sta.channel,
    config.sta.ssid,
    config.sta.password[0] ? "***" : "",
    wifi_auth_mode_str(config.sta.threshold.authmode)
  );

  if ((err = esp_wifi_set_config(WIFI_IF_STA, &config))) {
    LOG_ERROR("esp_wifi_set_config WIFI_IF_STA: %s", esp_err_to_name(err));
    return -1;
  }

  // state
  wifi_sta_connect = true;

  if (wifi_sta_started) {
    if ((err = esp_wifi_connect())) {
      LOG_ERROR("esp_wifi_connect: %s", esp_err_to_name(err));
      user_alert(USER_ALERT_ERROR_WIFI);
    } else {
      user_state(USER_STATE_CONNECTING);
    }
  } else {
    // start triggers WIFI_EVENT_STA_START -> esp_wifi_connect() -> USER_STATE_CONNECTING
  }

  return 0;
}

int wifi_disconnect()
{
  esp_err_t err;

  if (wifi_sta_connected) {
    LOG_INFO("disconnecting...");

    user_state(USER_STATE_DISCONNECTING);

    // disable reconnect before triggering event handler
    wifi_sta_connect = false;

    // disconnect triggers WIFI_EVENT_STA_DISCONNECTED -> USER_STATE_CONNECTED
    if ((err = esp_wifi_disconnect())) {
      LOG_ERROR("esp_wifi_disconnect: %s", esp_err_to_name(err));
      return -1;
    }

  } else if (wifi_sta_connect) {
    LOG_INFO("abort connect...");

    user_state(USER_STATE_DISCONNECTING);

    // disable reconnect before triggering event handler
    wifi_sta_connect = false;

    if ((err = esp_wifi_disconnect())) {
      LOG_ERROR("esp_wifi_disconnect: %s", esp_err_to_name(err));
      return -1;
    }

    // there will not be a WIFI_EVENT_STA_DISCONNECTED
    user_state(USER_STATE_DISCONNECTED);

  } else {
    LOG_WARN("not connected");

    wifi_sta_connect = false;

    return 1;
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

int disable_wifi()
{
  esp_err_t err;

  LOG_INFO("stopping and disabling...");

  if ((err = esp_wifi_stop())) {
    LOG_ERROR("esp_wifi_stop: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_set_mode(WIFI_MODE_NULL))) {
    LOG_ERROR("esp_wifi_set_mode: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}
