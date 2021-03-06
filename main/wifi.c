#include "wifi.h"

#include <logging.h>

#include <esp_err.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <tcpip_adapter.h>

const wifi_auth_mode_t authmode_threshold = WIFI_AUTH_WPA2_PSK;

int init_tcpip_adapter()
{
  tcpip_adapter_init();

  return 0;
}

int init_esp_event()
{
  esp_err_t err;

  if ((err = esp_event_loop_create_default())) {
    LOG_ERROR("esp_event_loop_create_default: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int init_wifi_sta()
{
  wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
  esp_err_t err;

  LOG_INFO("Start ESP8266 WiFI in Station mode");

  if ((err = esp_wifi_init(&init_config))) {
    LOG_ERROR("esp_wifi_init: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_set_storage(WIFI_STORAGE_RAM))) {
    LOG_ERROR("esp_wifi_set_storage: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_set_mode(WIFI_MODE_STA))) {
    LOG_ERROR("esp_wifi_set_mode WIFI_MODE_STA: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_start())) {
    LOG_ERROR("esp_wifi_start: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int init_wifi()
{

  if (init_tcpip_adapter()) {
    LOG_ERROR("init_tcpip_adapter");
    return -1;
  }

  if (init_esp_event()) {
    LOG_ERROR("init_esp_event");
    return -1;
  }

  if (init_wifi_sta()) {
    LOG_ERROR("init_wifi_sta");
    return -1;
  }

  return 0;
}

const char *wifi_auth_mode_str(wifi_auth_mode_t auth_mode) {
  switch (auth_mode) {
    case WIFI_AUTH_OPEN:            return "OPEN";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA-PSK";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2-PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/2-PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3-PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/3-PSK";
    default:                        return "?";
  }
}

const char *wifi_cipher_type_str(wifi_cipher_type_t cipher_type) {
  switch (cipher_type) {
    case WIFI_CIPHER_TYPE_NONE:         return "NONE";
    case WIFI_CIPHER_TYPE_WEP40:        return "WEP40";
    case WIFI_CIPHER_TYPE_WEP104:       return "WEP104";
    case WIFI_CIPHER_TYPE_TKIP:         return "TKIP";
    case WIFI_CIPHER_TYPE_CCMP:         return "CCMP";
    case WIFI_CIPHER_TYPE_TKIP_CCMP:    return "TKIP-CCMP";
    case WIFI_CIPHER_TYPE_AES_CMAC128:  return "AES-CMAC128";
    case WIFI_CIPHER_TYPE_UNKNOWN:      return "UNKNOWN";
    default:                            return "?";
  }
}

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

  LOG_INFO("Connect: ssid=%.32s password=%s",
    config->sta.ssid,
    config->sta.password[0] ? "***" : ""
  );

  if ((err = esp_wifi_set_config(ESP_IF_WIFI_STA, config))) {
    LOG_ERROR("esp_wifi_set_config: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_connect())) {
    LOG_ERROR("esp_wifi_connect: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int wifi_info()
{
  uint8_t mac[6];
  wifi_ap_record_t ap;
  esp_err_t err;

  if ((err = esp_wifi_get_mac(ESP_IF_WIFI_STA, mac))) {
    LOG_ERROR("esp_wifi_get_mac: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_sta_get_ap_info(&ap))) {
    if (err == ESP_ERR_WIFI_NOT_CONNECT) {
      LOG_WARN("esp_wifi_sta_get_ap_info: %s", esp_err_to_name(err));
    } else {
      LOG_ERROR("esp_wifi_sta_get_ap_info: %s", esp_err_to_name(err));
      return -1;
    }
  }

  printf("Station %02x:%02x:%02x:%02x:%02x:%02x: %s\n",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
    err ? "Not Connected" : "Connected"
  );

  if (!err) {
    printf("%20s: %02x:%02x:%02x:%02x:%02x:%02x\n", "BSSID",
      ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3], ap.bssid[4], ap.bssid[5]
    );
    printf("%20s: %.32s\n", "SSID", ap.ssid);
    printf("%20s: %d:%d\n", "Channel", ap.primary, ap.second);
    printf("%20s: %d\n", "RSSI", ap.rssi);
    printf("%20s: %s\n", "AuthMode", wifi_auth_mode_str(ap.authmode));
    printf("%20s: %s\n", "Pairwise Cipher", wifi_cipher_type_str(ap.pairwise_cipher));
    printf("%20s: %s\n", "Group Cipher", wifi_cipher_type_str(ap.group_cipher));
    printf("%20s: %s %s %s %s %s\n", "Flags",
      ap.phy_11b  ? "11b" : "",
      ap.phy_11g  ? "11g" : "",
      ap.phy_11n  ? "11n" : "",
      ap.phy_lr   ? "LR"  : "",
      ap.wps      ? "WPS" : ""
    );
  }

  return 0;
}

int wifi_scan_cmd(int argc, char **argv, void *ctx)
{
  wifi_scan_config_t scan_config = {};
  int err;

  if (argc >= 2 && (err = cmd_arg_str(argc, argv, 1, (const char **) &scan_config.ssid))) {
    return err;
  }

  if ((err = wifi_scan(&scan_config))) {
    LOG_ERROR("wifi_scan");
    return err;
  }

  return 0;
}

int wifi_connect_cmd(int argc, char **argv, void *ctx)
{
  wifi_config_t config = {};
  int err;

  if (argc >= 2 && (err = cmd_arg_strncpy(argc, argv, 1, (char *) config.ap.ssid, sizeof(config.sta.ssid)))) {
    return err;
  }
  if (argc >= 3 && (err = cmd_arg_strncpy(argc, argv, 2, (char *) config.ap.password, sizeof(config.sta.password)))) {
    return err;
  }

  if (config.sta.password[0]) {
    LOG_INFO("Using authmode threshold: %s", wifi_auth_mode_str(authmode_threshold));
    config.sta.threshold.authmode = authmode_threshold;
  }

  if ((err = wifi_connect(&config))) {
    LOG_ERROR("wifi_info");
    return err;
  }

  return 0;
}

int wifi_info_cmd(int argc, char **argv, void *ctx)
{
  int err;

  if ((err = wifi_info())) {
    LOG_ERROR("wifi_info");
    return err;
  }

  return 0;
}

const struct cmd wifi_commands[] = {
  { "scan",     wifi_scan_cmd,    .usage = "[SSID]",        .describe = "Scan available APs"  },
  { "connect",  wifi_connect_cmd, .usage = "[SSID] [PSK]",  .describe = "Connect AP"          },
  { "info",     wifi_info_cmd,    .usage = "",              .describe = "Show connected AP"   },
  {}
};

const struct cmdtab wifi_cmdtab = {
  .commands = wifi_commands,
};
