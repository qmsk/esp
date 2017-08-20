#include "user_config.h"
#include "logging.h"

#include <c_types.h>
#include <esp_misc.h>
#include <esp_wifi.h>
#include <esp_sta.h>

static void on_wifi_event(System_Event_t *event)
{
  switch (event->event_id) {
    case EVENT_STAMODE_SCAN_DONE: {
      Event_StaMode_ScanDone_t info = event->event_info.scan_done;
      LOG_INFO("scan done: status=%u", info.status);
    } break;

    case EVENT_STAMODE_CONNECTED: {
      Event_StaMode_Connected_t info = event->event_info.connected;

      LOG_INFO("connected: ssid=%s bssid=" MACSTR " channel=%u", info.ssid, MAC2STR(info.bssid), info.channel);
    } break;

    case EVENT_STAMODE_DISCONNECTED: {
      Event_StaMode_Disconnected_t info = event->event_info.disconnected;

      LOG_INFO("disconnected: ssid=%s reason=%u", info.ssid, info.reason);
    } break;

    case EVENT_STAMODE_AUTHMODE_CHANGE: {
      Event_StaMode_AuthMode_Change_t info = event->event_info.auth_change;

      LOG_INFO("auth mode change: mode=%u -> %u", info.old_mode, info.new_mode);
    } break;

    case EVENT_STAMODE_GOT_IP: {
      Event_StaMode_Got_IP_t info = event->event_info.got_ip;

      LOG_INFO("got ip: ip=" IPSTR " mask=" IPSTR " gw=" IPSTR,
          IP2STR(&info.ip),
          IP2STR(&info.mask),
          IP2STR(&info.gw)
      );
    } break;

    case EVENT_STAMODE_DHCP_TIMEOUT: {
      LOG_INFO("dhcp timeout");
    } break;

    default: {
      LOG_INFO("%u\n", event->event_id);
    }
  }
}

int init_wifi(const struct user_config *config)
{
  struct station_config wifi_station_config = { };

  snprintf(wifi_station_config.ssid, sizeof(wifi_station_config.ssid), "%s", config->wifi_ssid);
  snprintf(wifi_station_config.password, sizeof(wifi_station_config.password), "%s", config->wifi_password);

  LOG_INFO("config station mode with ssid=%s", wifi_station_config.ssid);

  if (!wifi_set_opmode(STATION_MODE)) {
    LOG_ERROR("wifi_set_opmode STATION_MODE");
    return -1;
  }

  if (!wifi_station_set_config(&wifi_station_config)) {
    LOG_ERROR("wifi_station_set_config");
    return -1;
  }

  if (!wifi_set_event_handler_cb(&on_wifi_event)) {
    LOG_ERROR("wifi_set_event_handler_cb");
    return -1;
  }

  return 0;
}

void print_wifi()
{
  WIFI_MODE wifi_mode = wifi_get_opmode();
  WIFI_PHY_MODE wifi_phymode = wifi_get_phy_mode();

  LOG_INFO("mode=%d phymode=%d", wifi_mode, wifi_phymode);

  if (wifi_mode == STATION_MODE) {
    uint8 wifi_sta_macaddr[6];
    struct station_config wifi_sta_config;
    struct ip_info wifi_sta_ipinfo;

    wifi_get_macaddr(STATION_IF, wifi_sta_macaddr);
    wifi_station_get_config(&wifi_sta_config);
    wifi_get_ip_info(STATION_IF, &wifi_sta_ipinfo);

    LOG_INFO("sta mac=" MACSTR, MAC2STR(wifi_sta_macaddr));
    LOG_INFO("sta ssid=%s password=%s", wifi_sta_config.ssid, wifi_sta_config.password);
    LOG_INFO("sta ip=" IPSTR, IP2STR(&wifi_sta_ipinfo.ip));
  }
}
