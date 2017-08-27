#define DEBUG

#include "wifi.h"
#include "logging.h"
#include "cli.h"

#include <c_types.h>
#include <esp_misc.h>
#include <esp_wifi.h>
#include <esp_sta.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

struct wifi_scan_event {
  int err;

  // XXX: this is horribly unsafe, no idea how the SDK manages this memory
  struct bss_info *bss_info;
};

struct wifi {
  xQueueHandle scan_queue;
} wifi;

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

  snprintf((char *) wifi_station_config.ssid, sizeof(wifi_station_config.ssid), "%s", config->wifi_ssid);
  snprintf((char *) wifi_station_config.password, sizeof(wifi_station_config.password), "%s", config->wifi_password);

  LOG_INFO("config station mode with ssid=%s", wifi_station_config.ssid);

  if ((wifi.scan_queue = xQueueCreate(1, sizeof(struct wifi_scan_event))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

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

void wifi_print()
{
  WIFI_MODE wifi_mode = wifi_get_opmode();
  WIFI_PHY_MODE wifi_phymode = wifi_get_phy_mode();

  cli_printf("wifi mode=%d phymode=%d\n", wifi_mode, wifi_phymode);

  if (wifi_mode == STATION_MODE) {
    uint8 wifi_sta_macaddr[6];
    struct station_config wifi_sta_config;
    struct ip_info wifi_sta_ipinfo;
    STATION_STATUS wifi_sta_status = wifi_station_get_connect_status();
    sint8 wifi_sta_rssi = wifi_station_get_rssi();
    enum dhcp_status wifi_dhcp_status = wifi_station_dhcpc_status();
    char *wifi_dhcp_hostname = wifi_station_get_hostname();

    wifi_get_macaddr(STATION_IF, wifi_sta_macaddr);
    wifi_station_get_config(&wifi_sta_config);
    wifi_get_ip_info(STATION_IF, &wifi_sta_ipinfo);

    cli_printf("wifi sta mac=" MACSTR "\n", MAC2STR(wifi_sta_macaddr));
    cli_printf("wifi sta ssid=%.32s password=%.64s\n", wifi_sta_config.ssid, wifi_sta_config.password);
    if (wifi_sta_config.bssid_set)
      cli_printf("wifi sta bssid=" MACSTR "\n", MAC2STR(wifi_sta_config.bssid));
    cli_printf("wifi sta rssi=%d\n", wifi_sta_rssi);
    cli_printf("wifi sta status=%d\n", wifi_sta_status);
    cli_printf("wifi sta ip=" IPSTR "\n", IP2STR(&wifi_sta_ipinfo.ip));
    cli_printf("wifi dhcp status=%d hostname=%s\n", wifi_dhcp_status, wifi_dhcp_hostname);
  }
}

void wifi_scan_done(void *arg, STATUS status) // SDK callback
{
  struct wifi_scan_event scan_event = { };

  LOG_DEBUG("task=%p status=%d", xTaskGetCurrentTaskHandle(), status);

  if (status != OK) {
    LOG_ERROR("wifi scan error: status=%d", status);
    scan_event.err = -1;
    scan_event.bss_info = NULL;
  } else {
    LOG_INFO("wifi scan ok");
    scan_event.err = 0;
    scan_event.bss_info = arg;
  }

  if (!xQueueOverwrite(wifi.scan_queue, &scan_event)) {
    LOG_ERROR("xQueueOverwrite");
  }
}

// @return <0 on error, >0 on timeout
int wifi_scan(struct wifi_scan_event *scan_event, portTickType timeout)
{
  struct scan_config wifi_scan_config = { };

  // XXX: racy?
  if (!xQueueReset(wifi.scan_queue)) {
    LOG_ERROR("xQueueReset");
  }

  if (!wifi_station_scan(&wifi_scan_config, &wifi_scan_done)) {
    LOG_ERROR("wifi_stations_scan");
    return -1;
  }

  if (!xQueuePeek(wifi.scan_queue, scan_event, timeout)) {
    return 1;
  }

  return 0;
}

void wifi_print_scan(struct wifi_scan_event *scan_event)
{
  uint count = 0;

  cli_printf("wifi scan: %32s %17s %4s %4s\n",
    "SSID",
    "BSSID",
    "CHAN",
    "RSSI"
  );

  for (struct bss_info *bss_info = scan_event->bss_info; bss_info; bss_info = STAILQ_NEXT(bss_info, next)) {
    cli_printf("wifi scan: %32.32s " MACSTR " %4u %+4d\n",
      bss_info->ssid,
      MAC2STR(bss_info->bssid),
      bss_info->channel,
      bss_info->rssi
    );
    count++;
  }

  cli_printf("wifi scan: total of %u APs\n", count);
}


int wifi_cmd_status(int argc, char **argv, void *ctx)
{
  if (argc != 1)
    return -CMD_ERR_ARGC;

  wifi_print();

  return 0;
}

int wifi_cmd_scan(int argc, char **argv, void *ctx)
{
  struct wifi_scan_event scan_event;
  int err;

  if (argc != 1)
    return -CMD_ERR_ARGC;

  if ((err = wifi_scan(&scan_event, 5000 / portTICK_RATE_MS)) < 0)
    return -CMD_ERR_FAILED;
  else if (err > 0)
    return -CMD_ERR_TIMEOUT;
  else if (scan_event.err)
    return -CMD_ERR_FAILED;

  wifi_print_scan(&scan_event);

  return 0;
}

const struct cmd wifi_commands[] = {
  { "status", wifi_cmd_status },
  { "scan",   wifi_cmd_scan   },
  {}
};

const struct cmdtab wifi_cmdtab = {
  .commands = wifi_commands,
};
