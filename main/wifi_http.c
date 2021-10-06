#include "wifi.h"
#include "http_routes.h"

#include <logging.h>
#include <json.h>
#include <json_lwip.h>
#include <system_wifi.h>

#include <esp_err.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define JSON_WRITE_MEMBER_BSSID(w, name, v) (\
  json_open_object_member((w), (name)) || \
  json_write_raw((w), "\"%02x:%02x:%02x:%02x:%02x:%02x\"", \
    v[0], v[1], v[2], v[3], v[4], v[5] \
  ))

static int write_json_response(struct http_response *response, int(*f)(struct json_writer *w))
{
  struct json_writer json_writer;
  FILE *file;
  int err;

  if ((err = http_response_start(response, HTTP_OK, NULL))) {
    LOG_WARN("http_response_start");
    return err;
  }

  if ((err = http_response_header(response, "Content-Type", "application/json"))) {
    LOG_WARN("http_response_header");
    return err;
  }

  if ((err = http_response_open(response, &file))) {
    LOG_WARN("http_response_open");
    return err;
  }

  if (json_writer_init(&json_writer, file)) {
    LOG_ERROR("json_writer_init");
    return -1;
  }

  if ((err = f(&json_writer))) {
    return -1;
  }

  if (fclose(file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

  return 0;
}

static int wifi_api_write_sta_ap(struct json_writer *w, const wifi_ap_record_t *ap)
{
  return (
    JSON_WRITE_MEMBER_BSSID(w, "bssid", ap->bssid) ||
    JSON_WRITE_MEMBER_RAW(w, "ssid", "\"%.32s\"", ap->ssid) ||
    JSON_WRITE_MEMBER_UINT(w, "channel", ap->primary) ||
    JSON_WRITE_MEMBER_INT(w, "rssi", ap->rssi) ||
    JSON_WRITE_MEMBER_STRING(w, "authmode", wifi_auth_mode_str(ap->authmode)) ||
    JSON_WRITE_MEMBER_STRING(w, "pairwise_cipher", wifi_cipher_type_str(ap->pairwise_cipher)) ||
    JSON_WRITE_MEMBER_STRING(w, "group_cipher", wifi_cipher_type_str(ap->group_cipher)) ||
    JSON_WRITE_MEMBER_BOOL(w, "phy_11b", ap->phy_11b) ||
    JSON_WRITE_MEMBER_BOOL(w, "phy_11g", ap->phy_11g) ||
    JSON_WRITE_MEMBER_BOOL(w, "phy_11n", ap->phy_11n) ||
    JSON_WRITE_MEMBER_BOOL(w, "phy_lr", ap->phy_lr) ||
    JSON_WRITE_MEMBER_BOOL(w, "wps", ap->wps)
  );
}

static int wifi_api_write_sta_object(struct json_writer *w)
{
  uint8_t mac[6];
  wifi_config_t config;
  wifi_ap_record_t ap;
  bool ap_connected;
  esp_err_t err;

  if ((err = esp_wifi_get_mac(ESP_IF_WIFI_STA, mac))) {
    LOG_ERROR("esp_wifi_get_mac: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_get_config(ESP_IF_WIFI_STA, &config))) {
    LOG_ERROR("esp_wifi_get_config: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_sta_get_ap_info(&ap))) {
    if (err == ESP_ERR_WIFI_NOT_CONNECT) {
      LOG_WARN("esp_wifi_sta_get_ap_info: %s", esp_err_to_name(err));
      ap_connected = false;
    } else {
      LOG_ERROR("esp_wifi_sta_get_ap_info: %s", esp_err_to_name(err));
      return -1;
    }
  } else {
    ap_connected = true;
  }

  return (
    JSON_WRITE_MEMBER_BSSID(w, "mac", mac) ||
    JSON_WRITE_MEMBER_OBJECT(w, "config", (
      JSON_WRITE_MEMBER_RAW(w, "ssid", "\"%.32s\"", config.sta.ssid) ||
      JSON_WRITE_MEMBER_RAW(w, "password", "\"%.64s\"", config.sta.password) ||
      JSON_WRITE_MEMBER_UINT(w, "channel", config.sta.channel) ||
      JSON_WRITE_MEMBER_UINT(w, "threshold_rssi", config.sta.threshold.rssi) ||
      JSON_WRITE_MEMBER_STRING(w, "threshold_authmode", wifi_auth_mode_str(config.sta.threshold.authmode))
    )) ||
    (ap_connected ? JSON_WRITE_MEMBER_OBJECT(w, "ap", wifi_api_write_sta_ap(w, &ap)) : JSON_WRITE_MEMBER_NULL(w, "ap"))
  );
}

static int wifi_api_write_ap_sta_object(struct json_writer *w, const wifi_sta_info_t *wifi_sta, const tcpip_adapter_sta_info_t *tcpip_sta)
{
  return (
    JSON_WRITE_MEMBER_BSSID(w, "mac", wifi_sta->mac) ||
    JSON_WRITE_MEMBER_BOOL(w, "phy_11b", wifi_sta->phy_11b) ||
    JSON_WRITE_MEMBER_BOOL(w, "phy_11g", wifi_sta->phy_11g) ||
    JSON_WRITE_MEMBER_BOOL(w, "phy_11n", wifi_sta->phy_11n) ||
    JSON_WRITE_MEMBER_BOOL(w, "phy_lr", wifi_sta->phy_lr) ||
    JSON_WRITE_MEMBER_IPV4(w, "ipv4", &tcpip_sta->ip)
  );
}

static int wifi_api_write_ap_sta_array(struct json_writer *w)
{
  wifi_sta_list_t wifi_sta_list;
  tcpip_adapter_sta_list_t tcpip_sta_list;
  esp_err_t err;

  if ((err = esp_wifi_ap_get_sta_list(&wifi_sta_list))) {
    LOG_ERROR("esp_wifi_ap_get_sta_list: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = tcpip_adapter_get_sta_list(&wifi_sta_list, &tcpip_sta_list))) {
    LOG_ERROR("tcpip_adapter_get_sta_list: %s", esp_err_to_name(err));
    return -1;
  }

  for (int i = 0; i < wifi_sta_list.num && i < tcpip_sta_list.num; i++) {
    if ((err = JSON_WRITE_OBJECT(w, wifi_api_write_ap_sta_object(w, &wifi_sta_list.sta[i], &tcpip_sta_list.sta[i])))) {
      LOG_ERROR("wifi_api_write_ap_sta_object");
      return err;
    }
  }

  return 0;
}

static int wifi_api_write_ap_object(struct json_writer *w)
{
  uint8_t mac[6];
  wifi_config_t config;
  esp_err_t err;

  if ((err = esp_wifi_get_mac(WIFI_IF_AP, mac))) {
    LOG_ERROR("esp_wifi_get_mac: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_get_config(WIFI_IF_AP, &config))) {
    LOG_ERROR("esp_wifi_get_config: %s", esp_err_to_name(err));
    return -1;
  }

  return (
    JSON_WRITE_MEMBER_BSSID(w, "mac", mac) ||
    JSON_WRITE_MEMBER_OBJECT(w, "config",
      JSON_WRITE_MEMBER_RAW(w, "ssid", "\"%.32s\"", config.ap.ssid) ||
      JSON_WRITE_MEMBER_RAW(w, "password", "\"%.64s\"", config.ap.password) ||
      JSON_WRITE_MEMBER_UINT(w, "channel", config.ap.channel) ||
      JSON_WRITE_MEMBER_STRING(w, "authmode", wifi_auth_mode_str(config.ap.authmode)) ||
      JSON_WRITE_MEMBER_BOOL(w, "ssid_hidden", config.ap.ssid_hidden) ||
      JSON_WRITE_MEMBER_UINT(w, "max_connection", config.ap.max_connection) ||
      JSON_WRITE_MEMBER_UINT(w, "beacon_interval_ms", config.ap.beacon_interval)
    ) ||
    JSON_WRITE_MEMBER_ARRAY(w, "sta", wifi_api_write_ap_sta_array(w))
  );
}

static int wifi_api_write(struct json_writer *w)
{
  wifi_mode_t mode;
  esp_err_t err;

  if ((err = esp_wifi_get_mode(&mode))) {
    LOG_ERROR("esp_wifi_get_mode: %s", esp_err_to_name(err));
    return -1;
  }

  return JSON_WRITE_OBJECT(w,
    JSON_WRITE_MEMBER_STRING(w, "mode", wifi_mode_str(mode)) ||
    ((mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA) ? JSON_WRITE_MEMBER_OBJECT(w, "sta", wifi_api_write_sta_object(w)) : 0) ||
    ((mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) ? JSON_WRITE_MEMBER_OBJECT(w, "ap", wifi_api_write_ap_object(w)) : 0)
  );
}

int write_wifi_scan_ap(wifi_ap_record_t *ap, void *ctx)
{
  struct json_writer *w = ctx;

  return JSON_WRITE_OBJECT(w, wifi_api_write_sta_ap(w, ap));
}

static int wifi_api_scan_write(struct json_writer *w)
{
  wifi_scan_config_t scan_config = {};
  int err;

  if ((err = json_open_array(w))) {
    LOG_ERROR("json_open_array");
    return err;
  }

  if ((err = wifi_scan(&scan_config, write_wifi_scan_ap, w))) {
    LOG_ERROR("wifi_scan");
    return err;
  }

  if ((err = json_close_array(w))) {
    LOG_ERROR("json_close_array");
    return err;
  }

  return 0;
}

int wifi_api_handler(struct http_request *request, struct http_response *response, void *ctx)
{
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = write_json_response(response, wifi_api_write))) {
    LOG_WARN("write_json_response -> wifi_api_write");
    return err;
  }

  return 0;
}

int wifi_api_scan_handler(struct http_request *request, struct http_response *response, void *ctx)
{
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = write_json_response(response, wifi_api_scan_write))) {
    LOG_WARN("write_json_response -> wifi_api_write");
    return err;
  }

  return 0;
}
