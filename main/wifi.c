#include "wifi.h"
#include "user_event.h"

#include <logging.h>
#include <system_wifi.h>

#include <esp_err.h>
#include <esp_wifi.h>
#include <tcpip_adapter.h>

int wifi_scan(const wifi_scan_config_t *scan_config)
{
  wifi_ap_record_t *wifi_ap_records;
  uint16_t number;
  esp_err_t err;

  LOG_INFO("start: ssid=%s",
    scan_config->ssid ? (const char *) scan_config->ssid : "*"
  );

  if ((err = esp_wifi_scan_start(scan_config, true))) {
    LOG_ERROR("esp_wifi_scan_start: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_scan_get_ap_num(&number))) {
    LOG_ERROR("esp_wifi_scan_get_ap_num: %s", esp_err_to_name(err));
    return -1;
  }

  LOG_INFO("Scanned %u APs", number);

  if (!(wifi_ap_records = calloc(number, sizeof(*wifi_ap_records)))) {
    LOG_ERROR("calloc(%d)", number);
    return -1;
  }

  if ((err = esp_wifi_scan_get_ap_records(&number, wifi_ap_records))) {
    LOG_ERROR("esp_wifi_scan_get_ap_records: %s", esp_err_to_name(err));
    goto error;
  }

  printf("%-17s\t%-32s\t%5s\t%-4s\t%10s\t%21s\t%s\n",
    "BSSID",
    "SSID",
    "CH:CH",
    "RSSI",
    "AUTHMODE",
    "PAIRW/GROUP CIPH",
    "FLAGS"
  );

  for (wifi_ap_record_t *ap = wifi_ap_records; ap < wifi_ap_records + number; ap++) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x\t%-32.32s\t%-2d:%-2d\t%-4d\t%-10s\t%-10s/%-10s\t%c%c%c%c%c\n",
      ap->bssid[0], ap->bssid[1], ap->bssid[2], ap->bssid[3], ap->bssid[4], ap->bssid[5],
      ap->ssid,
      ap->primary, ap->second,
      ap->rssi,
      wifi_auth_mode_str(ap->authmode),
      wifi_cipher_type_str(ap->pairwise_cipher),
      wifi_cipher_type_str(ap->group_cipher),
      ap->phy_11b ? 'b' : ' ',
      ap->phy_11g ? 'g' : ' ',
      ap->phy_11n ? 'n' : ' ',
      ap->phy_lr  ? 'L' : ' ',
      ap->wps     ? 'W' : ' '
    );
  }

error:
  free(wifi_ap_records);

  return err;
}

int wifi_connect(wifi_config_t *config)
{
  esp_err_t err;

  LOG_INFO("channel=%u ssid=%.32s password=%s authmode=%s",
    config->sta.channel,
    config->sta.ssid,
    config->sta.password[0] ? "***" : "",
    wifi_auth_mode_str(config->sta.threshold.authmode)
  );

  if ((err = esp_wifi_set_config(ESP_IF_WIFI_STA, config))) {
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
