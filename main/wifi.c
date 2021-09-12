#include "wifi.h"
#include "user_event.h"

#include <logging.h>

#include <esp_err.h>
#include <esp_wifi.h>
#include <tcpip_adapter.h>

const char *wifi_mode_str(wifi_mode_t mode)
{
  switch(mode) {
    case WIFI_MODE_NULL:      return "NULL";
    case WIFI_MODE_STA:       return "STA";
    case WIFI_MODE_AP:        return "AP";
    case WIFI_MODE_APSTA:     return "APSTA";
    default:                  return "?";
  }
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

const char *tcpip_adapter_if_str(tcpip_adapter_if_t tcpip_if)
{
  switch(tcpip_if) {
    case TCPIP_ADAPTER_IF_STA:    return "STA";
    case TCPIP_ADAPTER_IF_AP:     return "AP";
    case TCPIP_ADAPTER_IF_ETH:    return "ETH";
    case TCPIP_ADAPTER_IF_TEST:   return "TEST";
    default:                      return "?";
  }
}

const char *tcpip_adapter_dhcp_status_str(tcpip_adapter_dhcp_status_t status)
{
  switch (status) {
    case TCPIP_ADAPTER_DHCP_INIT:         return "INIT";
    case TCPIP_ADAPTER_DHCP_STARTED:      return "STARTED";
    case TCPIP_ADAPTER_DHCP_STOPPED:      return "STOPPED";
    default:                              return "?";
  }
}

static inline void print_ip4_info(const char *title, ip4_addr_t *ip)
{
  printf("\t%-20s: %u.%u.%u.%u\n", title,
    ip4_addr1(ip),
    ip4_addr2(ip),
    ip4_addr3(ip),
    ip4_addr4(ip)
  );
}

static inline void print_ip6_info(const char *title, ip6_addr_t *ip)
{
  printf("\t%-20s: %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n", title,
    IP6_ADDR_BLOCK1(ip),
    IP6_ADDR_BLOCK2(ip),
    IP6_ADDR_BLOCK3(ip),
    IP6_ADDR_BLOCK4(ip),
    IP6_ADDR_BLOCK5(ip),
    IP6_ADDR_BLOCK6(ip),
    IP6_ADDR_BLOCK7(ip),
    IP6_ADDR_BLOCK8(ip)
 );
}

static inline void print_ip_info(const char *title, ip_addr_t *ip)
{
  if (IP_IS_V4(ip)) {
    print_ip4_info(title, ip_2_ip4(ip));
  } else if (IP_IS_V6(ip)) {
    print_ip6_info(title, ip_2_ip6(ip));
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

  LOG_INFO("ssid=%.32s password=%s authmode=%s",
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

  user_event(USER_EVENT_CONNECTING);

  return 0;
}

static int wifi_info_null()
{
  printf("Wifi OFF\n");

  return 0;
}

static int wifi_info_sta()
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


static int wifi_info_ap()
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

int wifi_info()
{
  wifi_mode_t mode;
  esp_err_t err;

  if ((err = esp_wifi_get_mode(&mode))) {
    LOG_ERROR("esp_wifi_get_mode: %s", esp_err_to_name(err));
    return -1;
  }

  switch (mode) {
    case WIFI_MODE_NULL:
      return wifi_info_null();

    case WIFI_MODE_STA:
      return wifi_info_sta();

    case WIFI_MODE_AP:
      return wifi_info_ap();

    default:
      LOG_ERROR("Unknown wifi mode=%d", mode);
      return -1;
  }
}

int tcpip_adapter_info(tcpip_adapter_if_t tcpip_if)
{
  const char *hostname;
  tcpip_adapter_dhcp_status_t dhcp_status;
  tcpip_adapter_ip_info_t ip_info;
  tcpip_adapter_dns_info_t dns_info_main, dns_info_backup, dns_info_fallback;
  esp_err_t err;

  if (!tcpip_adapter_is_netif_up(tcpip_if)) {
    printf("TCP/IP %s: DOWN\n", tcpip_adapter_if_str(tcpip_if));
    return 0;
  }

  if ((err = tcpip_adapter_get_hostname(tcpip_if, &hostname))) {
    LOG_ERROR("tcpip_adapter_get_hostname: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = tcpip_adapter_dhcpc_get_status(tcpip_if, &dhcp_status))) {
    LOG_ERROR("tcpip_adapter_dhcpc_get_status: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = tcpip_adapter_get_ip_info(tcpip_if, &ip_info))) {
    LOG_ERROR("tcpip_adapter_get_ip_info: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = tcpip_adapter_get_dns_info(tcpip_if, TCPIP_ADAPTER_DNS_MAIN, &dns_info_main))) {
    LOG_ERROR("tcpip_adapter_get_dns_info TCPIP_ADAPTER_DNS_MAIN: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = tcpip_adapter_get_dns_info(tcpip_if, TCPIP_ADAPTER_DNS_BACKUP, &dns_info_backup))) {
    LOG_ERROR("tcpip_adapter_get_dns_info TCPIP_ADAPTER_DNS_BACKUP: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = tcpip_adapter_get_dns_info(tcpip_if, TCPIP_ADAPTER_DNS_FALLBACK, &dns_info_fallback))) {
    LOG_ERROR("tcpip_adapter_get_dns_info TCPIP_ADAPTER_DNS_FALLBACK: %s", esp_err_to_name(err));
    return -1;
  }

  printf("TCP/IP %s: UP\n", tcpip_adapter_if_str(tcpip_if));
  printf("\t%-20s: %s\n", "Hostname", hostname ? hostname : "(null)");
  printf("\t%-20s: %s\n", "DHCP Client", tcpip_adapter_dhcp_status_str(dhcp_status));
  print_ip4_info("IP", &ip_info.ip);
  print_ip4_info("Netmask", &ip_info.netmask);
  print_ip4_info("Gateway", &ip_info.gw);
  print_ip_info("DNS (main)", &dns_info_main.ip);
  print_ip_info("DNS (backup)", &dns_info_backup.ip);
  print_ip_info("DNS (fallback)", &dns_info_fallback.ip);

  return 0;
}
