#include <c_types.h>
#include <esp_misc.h>
#include <esp_wifi.h>
#include <esp_sta.h>
#include "user_config.h"

static void on_wifi_event(System_Event_t *event)
{
  printf("INFO wifi event: id=%d\n", event->event_id);
}

int init_wifi(const struct user_config *config)
{
  struct station_config wifi_station_config = { };

  snprintf(wifi_station_config.ssid, sizeof(wifi_station_config.ssid), "%s", config->wifi_ssid);
  snprintf(wifi_station_config.password, sizeof(wifi_station_config.password), "%s", config->wifi_password);

  printf("INFO wifi: config station mode with ssid=%s\n", wifi_station_config.ssid);

  if (!wifi_set_opmode(STATION_MODE)) {
    printf("ERROR wifi: set station mode\n");
    return -1;
  }

  if (!wifi_station_set_config(&wifi_station_config)) {
    printf("ERROR wifi: set station config\n");
    return -1;
  }

  if (!wifi_set_event_handler_cb(&on_wifi_event)) {
    printf("ERROR wifi: set event handler\n");
    return -1;
  }

  return 0;
}

void print_wifi()
{
  WIFI_MODE wifi_mode = wifi_get_opmode();
  WIFI_PHY_MODE wifi_phymode = wifi_get_phy_mode();

  printf("INFO wifi: mode=%d phymode=%d\n", wifi_mode, wifi_phymode);

  if (wifi_mode == STATION_MODE) {
    uint8 wifi_sta_macaddr[6];
    struct station_config wifi_sta_config;
    struct ip_info wifi_sta_ipinfo;

    wifi_get_macaddr(STATION_IF, wifi_sta_macaddr);
    wifi_station_get_config(&wifi_sta_config);
    wifi_get_ip_info(STATION_IF, &wifi_sta_ipinfo);

    printf("INFO wifi sta: mac=" MACSTR "\n", MAC2STR(wifi_sta_macaddr));
    printf("INFO wifi sta: ssid=%s password=%s\n", wifi_sta_config.ssid, wifi_sta_config.password);
    printf("INFO wifi sta: ip=" IPSTR "\n", IP2STR(&wifi_sta_ipinfo.ip));
  }
}
