#include "wifi.h"
#include "wifi_cmd.h"

#include <logging.h>
#include <system_wifi.h>

#include <esp_wifi.h>

int wifi_start_cmd(int argc, char **argv, void *ctx)
{
  int err;

  if ((err = start_wifi())) {
    LOG_ERROR("start_wifi");
    return err;
  }

  return 0;
}

int wifi_stop_cmd(int argc, char **argv, void *ctx)
{
  int err;

  if ((err = stop_wifi())) {
    LOG_ERROR("stop_wifi");
    return err;
  }

  return 0;
}

int print_wifi_scan(wifi_ap_record_t *ap, void *ctx)
{
  int *countp = ctx;

  // print header before first line
  if ((*countp)++ == 0) {
    printf("%-17s\t%-32s\t%5s\t%-4s\t%10s\t%21s\t%s\n",
      "BSSID",
      "SSID",
      "CH:CH",
      "RSSI",
      "AUTHMODE",
      "PAIRW/GROUP CIPH",
      "FLAGS"
    );
  }

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

  return 0;
}

int wifi_scan_cmd(int argc, char **argv, void *ctx)
{
  wifi_scan_config_t scan_config = {};
  int count = 0;
  int err;

  if (argc >= 2 && (err = cmd_arg_str(argc, argv, 1, (const char **) &scan_config.ssid))) {
    return err;
  }

  if ((err = wifi_scan(&scan_config, print_wifi_scan, &count))) {
    LOG_ERROR("wifi_scan");
    return err;
  }

  return 0;
}

int wifi_connect_cmd(int argc, char **argv, void *ctx)
{
  wifi_sta_config_t sta_config = {};
  int err;

  if (argc >= 2 && (err = cmd_arg_strncpy(argc, argv, 1, (char *) sta_config.ssid, sizeof(sta_config.ssid)))) {
    return err;
  }
  if (argc >= 3 && (err = cmd_arg_strncpy(argc, argv, 2, (char *) sta_config.password, sizeof(sta_config.password)))) {
    return err;
  }

  if (sta_config.password[0]) {
    sta_config.threshold.authmode = WIFI_AUTHMODE_THRESHOLD;
  }

  if ((err = wifi_connect(&sta_config))) {
    LOG_ERROR("wifi_connect");
    return err;
  }

  return 0;
}

int wifi_listen_cmd(int argc, char **argv, void *ctx)
{
  wifi_ap_config_t ap_config = {};
  int err;

  if (argc >= 2 && (err = cmd_arg_strncpy(argc, argv, 1, (char *) ap_config.ssid, sizeof(ap_config.ssid)))) {
    return err;
  }
  if (argc >= 3 && (err = cmd_arg_strncpy(argc, argv, 2, (char *) ap_config.password, sizeof(ap_config.password)))) {
    return err;
  }

  if (ap_config.password[0]) {
    ap_config.authmode = WIFI_AUTHMODE_THRESHOLD;
  }

  if ((err = wifi_listen(&ap_config))) {
    LOG_ERROR("wifi_listen");
    return err;
  }

  return 0;
}

int wifi_disconnect_cmd(int argc, char **argv, void *ctx)
{
  int err;

  if ((err = wifi_disconnect())) {
    LOG_ERROR("wifi_disconnect");
    return err;
  }

  return 0;
}

int wifi_reconnect_cmd(int argc, char **argv, void *ctx)
{
  int err;

  if ((err = wifi_reconnect())) {
    LOG_ERROR("wifi_reconnect");
    return err;
  }

  return 0;
}

int wifi_disable_cmd(int argc, char **argv, void *ctx)
{
  int err;

  if ((err = wifi_disable())) {
    LOG_ERROR("wifi_disable");
    return err;
  }

  return 0;
}

static int print_wifi_info_null()
{
  printf("Wifi OFF\n");

  return 0;
}

static int print_wifi_info_sta()
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

  printf("WiFi STA %02x:%02x:%02x:%02x:%02x:%02x: %s\n",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
    err ? "Not Connected" : "Connected"
  );

  if (!err) {
    printf("\t%-20s: %02x:%02x:%02x:%02x:%02x:%02x\n", "BSSID",
      ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3], ap.bssid[4], ap.bssid[5]
    );
    printf("\t%-20s: %.32s\n", "SSID", ap.ssid);
    printf("\t%-20s: %d:%d\n", "Channel", ap.primary, ap.second);
    printf("\t%-20s: %d\n", "RSSI", ap.rssi);
    printf("\t%-20s: %s\n", "AuthMode", wifi_auth_mode_str(ap.authmode));
    printf("\t%-20s: %s\n", "Pairwise Cipher", wifi_cipher_type_str(ap.pairwise_cipher));
    printf("\t%-20s: %s\n", "Group Cipher", wifi_cipher_type_str(ap.group_cipher));
    printf("\t%-20s: %s %s %s %s %s\n", "Flags",
      ap.phy_11b  ? "11b" : "",
      ap.phy_11g  ? "11g" : "",
      ap.phy_11n  ? "11n" : "",
      ap.phy_lr   ? "LR"  : "",
      ap.wps      ? "WPS" : ""
    );
  }

  return 0;
}

static int print_wifi_info_ap()
{
  uint8_t mac[6];
  wifi_config_t config;
  wifi_sta_list_t wifi_sta_list;
  tcpip_adapter_sta_list_t tcpip_sta_list;
  esp_err_t err;

  if ((err = esp_wifi_get_mac(WIFI_IF_AP, mac))) {
    LOG_ERROR("esp_wifi_get_mac: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_get_config(WIFI_IF_AP, &config))) {
    LOG_ERROR("esp_wifi_get_config: %s", esp_err_to_name(err));
    return -1;
  }

  printf("WiFi AP %02x:%02x:%02x:%02x:%02x:%02x:\n",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
  );

  printf("\t%-20s: %.32s\n", "SSID", config.ap.ssid);
  printf("\t%-20s: %.32s\n", "Password", "***");
  printf("\t%-20s: %d\n", "Channel", config.ap.channel);
  printf("\t%-20s: %s\n", "AuthMode", wifi_auth_mode_str(config.ap.authmode));
  printf("\t%-20s: %s\n", "Hidden", config.ap.ssid_hidden ? "true" : "false");
  printf("\t%-20s: %d\n", "Max Connection", config.ap.max_connection);
  printf("\t%-20s: %d\n", "Beacon Interval", config.ap.beacon_interval);

  /* connected STA info */
  if ((err = esp_wifi_ap_get_sta_list(&wifi_sta_list))) {
    LOG_ERROR("esp_wifi_ap_get_sta_list: %s", esp_err_to_name(err));
    return -1;
  }
  if ((err = tcpip_adapter_get_sta_list(&wifi_sta_list, &tcpip_sta_list))) {
    LOG_ERROR("tcpip_adapter_get_sta_list: %s", esp_err_to_name(err));
    return -1;
  }

  printf("WiFi AP: %d stations connected\n", wifi_sta_list.num);

  for (int i = 0; i < wifi_sta_list.num && i < tcpip_sta_list.num; i++) {
    wifi_sta_info_t *wifi_sta_info = &wifi_sta_list.sta[i];
    tcpip_adapter_sta_info_t *tcpip_sta_info = &tcpip_sta_list.sta[i];

    printf("\t%02x:%02x:%02x:%02x:%02x:%02x\t%c%c%c%c\t%u.%u.%u.%u\n",
      wifi_sta_info->mac[0], wifi_sta_info->mac[1], wifi_sta_info->mac[2], wifi_sta_info->mac[3], wifi_sta_info->mac[4], wifi_sta_info->mac[5],
      wifi_sta_info->phy_11b ? 'b' : ' ',
      wifi_sta_info->phy_11g ? 'g' : ' ',
      wifi_sta_info->phy_11n ? 'n' : ' ',
      wifi_sta_info->phy_lr  ? 'L' : ' ',
      ip4_addr1(&tcpip_sta_info->ip),
      ip4_addr2(&tcpip_sta_info->ip),
      ip4_addr3(&tcpip_sta_info->ip),
      ip4_addr4(&tcpip_sta_info->ip)
    );
  }

  return 0;
}

int wifi_info_cmd(int argc, char **argv, void *ctx)
{
  wifi_mode_t mode;
  esp_err_t err;

  if ((err = esp_wifi_get_mode(&mode))) {
    LOG_ERROR("esp_wifi_get_mode: %s", esp_err_to_name(err));
    return -1;
  }

  switch (mode) {
    case WIFI_MODE_NULL:
      return print_wifi_info_null();

    case WIFI_MODE_STA:
      return print_wifi_info_sta();

    case WIFI_MODE_AP:
      return print_wifi_info_ap();

    case WIFI_MODE_APSTA:
      return print_wifi_info_ap() || print_wifi_info_sta();

    default:
      LOG_ERROR("Unknown wifi mode=%d", mode);
      return -1;
  }

  return 0;
}

const struct cmd wifi_commands[] = {
  { "start",      wifi_start_cmd,       .usage = "",              .describe = "Start WiFi system"   },
  { "stop",       wifi_stop_cmd,        .usage = "",              .describe = "Stop WiFi system"    },
  { "scan",       wifi_scan_cmd,        .usage = "[SSID]",        .describe = "Scan available APs"  },
  { "listen",     wifi_listen_cmd,      .usage = "[SSID] [PSK]",  .describe = "Establish AP"        },
  { "connect",    wifi_connect_cmd,     .usage = "[SSID] [PSK]",  .describe = "Connect to AP"       },
  { "disconnect", wifi_disconnect_cmd,  .usage = "",              .describe = "Disconnect from AP"  },
  { "reconnect",  wifi_reconnect_cmd,   .usage = "",              .describe = "Reconnect to AP"     },
  { "disable",    wifi_disable_cmd,     .usage = "",              .describe = "Disable STA/AP"      },
  { "info",       wifi_info_cmd,        .usage = "",              .describe = "Show connected AP"   },
  {}
};

const struct cmdtab wifi_cmdtab = {
  .commands = wifi_commands,
};