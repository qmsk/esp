#include "wifi.h"
#include "wifi_state.h"
#include "wifi_interface.h"
#include "user.h"

#include <logging.h>
#include <system_wifi.h>

#include <esp_err.h>
#include <esp_event.h>
#include <esp_wifi_types.h>

// TODO: deprecated
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
  esp_err_t err;

  wifi_sta_started = true;

  on_wifi_interface_up(WIFI_IF_STA, false); // not yet connected

  if (wifi_sta_connect) {
    LOG_INFO("connect...");

    user_state(USER_STATE_CONNECTING);

    if ((err = esp_wifi_connect())) {
      LOG_ERROR("esp_wifi_connect: %s", esp_err_to_name(err));
      user_alert(USER_ALERT_ERROR_WIFI);
    }

  } else {
    LOG_INFO("idle");
  }
}

static void on_sta_stop()
{
  LOG_INFO("connect=%d connected=%d", wifi_sta_connect, wifi_sta_connected);

  wifi_sta_started = false;

  on_wifi_interface_down(WIFI_IF_STA);

  user_state(USER_STATE_STOPPED);
}

static void on_sta_connected(wifi_event_sta_connected_t *event)
{
  int rssi = 0;
  wifi_phy_mode_t phymode = -1;
  esp_err_t err;

  if ((err = esp_wifi_sta_get_rssi(&rssi))) {

  }

  if ((err = esp_wifi_sta_get_negotiated_phymode(&phymode))) {

  }

  LOG_INFO("ssid=%.32s bssid=%02x:%02x:%02x:%02x:%02x:%02x channel=%u rssi=%d phymode=%s authmode=%s",
    event->ssid,
    event->bssid[0], event->bssid[1], event->bssid[2], event->bssid[3], event->bssid[4], event->bssid[5],
    event->channel,
    rssi,
    wifi_phy_mode_str(phymode),
    wifi_auth_mode_str(event->authmode)
  );

  wifi_sta_connected = true;

  // wait for netif -> IP_EVENT_STA_GOT_IP before USER_STATE_CONNECTED
}

static void on_sta_disconnected(wifi_event_sta_disconnected_t *event)
{
  esp_err_t err;

  LOG_INFO("ssid=%.32s bssid=%02x:%02x:%02x:%02x:%02x:%02x reason=%s",
    event->ssid,
    event->bssid[0], event->bssid[1], event->bssid[2], event->bssid[3], event->bssid[4], event->bssid[5],
    wifi_err_reason_str(event->reason)
  );

  if (wifi_sta_connect) {
    if (wifi_sta_connected) {
      LOG_INFO("reconnect...");
    } else {
      LOG_INFO("retry connect...");
    }

    // reconnect
    if ((err = esp_wifi_connect())) {
      LOG_ERROR("esp_wifi_connect: %s", esp_err_to_name(err));
      user_alert(USER_ALERT_ERROR_WIFI);
      user_state(USER_STATE_DISCONNECTED);
    } else {
      user_state(USER_STATE_CONNECTING);
    }
  } else {
    LOG_INFO("disconnected");

    user_state(USER_STATE_DISCONNECTED);
  }

  wifi_sta_connected = false;
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
  LOG_INFO("listen=%d started=%d connected=%d", wifi_ap_listen, wifi_ap_started, wifi_ap_connected);

  if (wifi_ap_listen) {
    user_state(USER_STATE_DISCONNECTED);
  }

  wifi_ap_started = true;

  on_wifi_interface_up(WIFI_IF_AP, true);
}

static void on_ap_stop()
{
  LOG_INFO("listen=%d started=%d connected=%d", wifi_ap_listen, wifi_ap_started, wifi_ap_connected);

  user_state(USER_STATE_STOPPED);

  wifi_ap_started = false;
  wifi_ap_connected = 0;

  on_wifi_interface_down(WIFI_IF_AP);
}

static void on_ap_sta_connected(wifi_event_ap_staconnected_t *event)
{
  LOG_INFO("aid=%d mac=%02x:%02x:%02x:%02x:%02x:%02x",
    event->aid,
    event->mac[0], event->mac[1], event->mac[2], event->mac[3], event->mac[4], event->mac[5]
  );

  wifi_ap_connected++;

  LOG_INFO("listen=%d started=%d connected=%d", wifi_ap_listen, wifi_ap_started, wifi_ap_connected);

  user_state(USER_STATE_CONNECTED);
}

static void on_ap_sta_disconnected(wifi_event_ap_stadisconnected_t *event)
{
  LOG_INFO("aid=%d mac=%02x:%02x:%02x:%02x:%02x:%02x",
    event->aid,
    event->mac[0], event->mac[1], event->mac[2], event->mac[3], event->mac[4], event->mac[5]
  );

  if (wifi_ap_connected == 0) {
    LOG_WARN("wifi_ap_connected=%d state mismatch", wifi_ap_connected);
  } else {
    wifi_ap_connected--;

    LOG_INFO("listen=%d started=%d connected=%d", wifi_ap_listen, wifi_ap_started, wifi_ap_connected);
  }

  if (!wifi_ap_connected) {
    user_state(USER_STATE_DISCONNECTED);
  }
}

static void on_ap_probe_req_recved(wifi_event_ap_probe_req_rx_t *event)
{
  LOG_INFO("rssi=%d mac=%02x:%02x:%02x:%02x:%02x:%02x",
    event->rssi,
    event->mac[0], event->mac[1], event->mac[2], event->mac[3], event->mac[4], event->mac[5]
  );
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

  if ((err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL))) {
    LOG_ERROR("esp_event_handler_register WIFI_EVENT: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}
