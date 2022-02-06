#include "wifi.h"
#include "user_event.h"
#include "system.h"

#include <logging.h>
#include <system_interfaces.h>
#include <system_wifi.h>

#include <esp_err.h>
#include <esp_event.h>

static void on_wifi_ready()
{
  LOG_INFO(" ");
}

static void on_scan_done(wifi_event_sta_scan_done_t *event)
{
  LOG_INFO("status=%s number=%u", event->status ? "failure" : "success", event->number);
}

static void on_sta_start()
{
  LOG_INFO(" ");
}

static void on_sta_stop()
{
  LOG_INFO(" ");
}

static void on_sta_connected(wifi_event_sta_connected_t *event)
{
  LOG_INFO("ssid=%.32s bssid=%02x:%02x:%02x:%02x:%02x:%02x channel=%u authmode=%s",
    event->ssid,
    event->bssid[0], event->bssid[1], event->bssid[2], event->bssid[3], event->bssid[4], event->bssid[5],
    event->channel,
    wifi_auth_mode_str(event->authmode)
  );
}

static void on_sta_disconnected(wifi_event_sta_disconnected_t *event)
{
  LOG_INFO("ssid=%.32s bssid=%02x:%02x:%02x:%02x:%02x:%02x reason=%s",
    event->ssid,
    event->bssid[0], event->bssid[1], event->bssid[2], event->bssid[3], event->bssid[4], event->bssid[5],
    wifi_err_reason_str(event->reason)
  );

  user_state(USER_STATE_DISCONNECTED);

  start_wifi_reconnect();
}

static void on_sta_authmode_change(wifi_event_sta_authmode_change_t *event)
{
  LOG_INFO("old=%s new=%s", wifi_auth_mode_str(event->old_mode), wifi_auth_mode_str(event->new_mode));
}

static void on_sta_bss_rssi_low(wifi_event_bss_rssi_low_t *event)
{
  LOG_INFO("rssi=%d", event->rssi);
}

static void on_ap_start()
{
  tcpip_adapter_ip_info_t ip_info = {};
  esp_err_t err;

  if ((err = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info))) {
    LOG_WARN("tcpip_adapter_get_ip_info TCPIP_ADAPTER_IF_AP: %s", esp_err_to_name(err));
  }

  LOG_INFO("ip=%d.%d.%d.%d",
    ip4_addr1_val(ip_info.ip),
    ip4_addr2_val(ip_info.ip),
    ip4_addr3_val(ip_info.ip),
    ip4_addr4_val(ip_info.ip)
  );

  user_state(USER_STATE_CONNECTED);
  update_user_ipv4_address(ip_info.ip);
}

static void on_ap_stop()
{
  LOG_INFO(" ");

  user_state(USER_STATE_DISCONNECTED);
}

static void on_ap_sta_connected(wifi_event_ap_staconnected_t *event)
{
  LOG_INFO("aid=%d mac=%02x:%02x:%02x:%02x:%02x:%02x",
    event->aid,
    event->mac[0], event->mac[1], event->mac[2], event->mac[3], event->mac[4], event->mac[5]
  );
}

static void on_ap_sta_disconnected(wifi_event_ap_stadisconnected_t *event)
{
  LOG_INFO("aid=%d mac=%02x:%02x:%02x:%02x:%02x:%02x",
    event->aid,
    event->mac[0], event->mac[1], event->mac[2], event->mac[3], event->mac[4], event->mac[5]
  );
}

static void on_ap_probe_req_recved(wifi_event_ap_probe_req_rx_t *event)
{
  LOG_INFO("rssi=%d mac=%02x:%02x:%02x:%02x:%02x:%02x",
    event->rssi,
    event->mac[0], event->mac[1], event->mac[2], event->mac[3], event->mac[4], event->mac[5]
  );
}

static void on_ip_sta_got_ip(ip_event_got_ip_t *event)
{
  LOG_INFO("if=%s ip=%d.%d.%d.%d changed=%s",
    tcpip_adapter_if_str(event->if_index),
    ip4_addr1_val(event->ip_info.ip), ip4_addr2_val(event->ip_info.ip), ip4_addr3_val(event->ip_info.ip), ip4_addr4_val(event->ip_info.ip),
    event->ip_changed ? "true" : "false"
  );

  user_state(USER_STATE_CONNECTED);
  update_user_ipv4_address(event->ip_info.ip);
}

static void on_ip_sta_lost_ip()
{
  LOG_INFO(" ");

  user_state(USER_STATE_DISCONNECTED);

  start_wifi_reconnect();
}

static void on_ip_ap_sta_ip_assigned(ip_event_ap_staipassigned_t *event)
{
  LOG_INFO("ip=%d.%d.%d.%d",
    ip4_addr1_val(event->ip), ip4_addr2_val(event->ip), ip4_addr3_val(event->ip), ip4_addr4_val(event->ip)
  );
}

static void on_ip_got_ip6(ip_event_got_ip6_t *event)
{
  LOG_INFO("if=%s ip=%4x:%4x:%4x:%4x:%4x:%4x:%4x:%4x",
    tcpip_adapter_if_str(event->if_index),
    IP6_ADDR_BLOCK1(&event->ip6_info.ip),
    IP6_ADDR_BLOCK2(&event->ip6_info.ip),
    IP6_ADDR_BLOCK3(&event->ip6_info.ip),
    IP6_ADDR_BLOCK4(&event->ip6_info.ip),
    IP6_ADDR_BLOCK5(&event->ip6_info.ip),
    IP6_ADDR_BLOCK6(&event->ip6_info.ip),
    IP6_ADDR_BLOCK7(&event->ip6_info.ip),
    IP6_ADDR_BLOCK8(&event->ip6_info.ip)
  );

}

static void tcpip_adapter_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  switch (event_id) {
    case IP_EVENT_STA_GOT_IP:
      return on_ip_sta_got_ip(event_data);

    case IP_EVENT_STA_LOST_IP:
      return on_ip_sta_lost_ip();

    case IP_EVENT_AP_STAIPASSIGNED:
      return on_ip_ap_sta_ip_assigned(event_data);

    case IP_EVENT_GOT_IP6:
      return on_ip_got_ip6(event_data);
  }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  switch (event_id) {
    case WIFI_EVENT_WIFI_READY:
      return on_wifi_ready();

    case WIFI_EVENT_SCAN_DONE:
      return on_scan_done(event_data);

    case WIFI_EVENT_STA_START:
      return on_sta_start();

    case WIFI_EVENT_STA_STOP:
      return on_sta_stop();

    case WIFI_EVENT_STA_CONNECTED:
      return on_sta_connected(event_data);

    case WIFI_EVENT_STA_DISCONNECTED:
      return on_sta_disconnected(event_data);

    case WIFI_EVENT_STA_AUTHMODE_CHANGE:
      return on_sta_authmode_change(event_data);

    case WIFI_EVENT_STA_BSS_RSSI_LOW:
      return on_sta_bss_rssi_low(event_data);

    case WIFI_EVENT_AP_START:
      return on_ap_start(event_data);

    case WIFI_EVENT_AP_STOP:
      return on_ap_stop(event_data);

    case WIFI_EVENT_AP_STACONNECTED:
      return on_ap_sta_connected(event_data);

    case WIFI_EVENT_AP_STADISCONNECTED:
      return on_ap_sta_disconnected(event_data);

    case WIFI_EVENT_AP_PROBEREQRECVED:
      return on_ap_probe_req_recved(event_data);
  }
}

int init_wifi_events()
{
  esp_err_t err;

  if ((err = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &tcpip_adapter_event_handler, NULL))) {
    LOG_ERROR("esp_event_handler_register WIFI_EVENT: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL))) {
    LOG_ERROR("esp_event_handler_register WIFI_EVENT: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}
