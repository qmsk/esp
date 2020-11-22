#define DEBUG

#include "wifi.h"
#include "wifi_cmd.h"
#include "wifi_config.h"

#include <lib/cli.h>
#include <lib/logging.h>

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
  struct user_info *user_info;
  user_func_t user_func;

  xQueueHandle scan_queue;
} wifi;

static void on_wifi_event(System_Event_t *event);


int wifi_init(struct wifi *wifi, struct user_info *user_info, user_func_t user_func)
{
  wifi->user_info = user_info;
  wifi->user_func = user_func;

  if ((wifi->scan_queue = xQueueCreate(1, sizeof(struct wifi_scan_event))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  if (!wifi_set_event_handler_cb(&on_wifi_event)) {
    LOG_ERROR("wifi_set_event_handler_cb");
    return -1;
  }

  // enable auto-connect
  if (!wifi_station_set_auto_connect(true)) {
    LOG_ERROR("wifi_station_set_auto_connect(true)");
  }

  wifi_get_macaddr(STATION_IF, user_info->mac);

  return 0;
}

void wifi_connecting(struct wifi *wifi)
{
  LOG_INFO("");

  if (wifi->user_func) {
    wifi->user_func(wifi->user_info, WIFI_CONNECTING);
  }
}

void wifi_connected(struct wifi *wifi)
{
  struct ip_info wifi_sta_ipinfo;
  char *wifi_sta_hostname;

  wifi_get_ip_info(STATION_IF, &wifi_sta_ipinfo);
  wifi_sta_hostname = wifi_station_get_hostname();

  wifi->user_info->ip = wifi_sta_ipinfo.ip;
  snprintf(wifi->user_info->hostname, sizeof(wifi->user_info->hostname), "%s", wifi_sta_hostname);

  LOG_INFO("hostname=%s ip=" IPSTR, wifi->user_info->hostname, IP2STR(&wifi->user_info->ip));

  wifi->user_info->connected = true;

  if (wifi->user_func) {
    wifi->user_func(wifi->user_info, WIFI_CONNECTED);
  }
}

void wifi_disconnected(struct wifi *wifi)
{
  LOG_INFO("");

  wifi->user_info->connected = false;

  if (wifi->user_func) {
    wifi->user_func(wifi->user_info, WIFI_DISCONNECTED);
  }
}

void wifi_update_status(struct wifi *wifi, STATION_STATUS status)
{
  switch (status) {
    case STATION_IDLE:
      LOG_INFO("station idle");
      wifi_disconnected(wifi);
      break;

    case STATION_CONNECTING:
      LOG_INFO("connecting");
      wifi_connecting(wifi);
      break;

    case STATION_WRONG_PASSWORD:
      LOG_WARN("wrong password");
      wifi_disconnected(wifi);
      break;

    case STATION_NO_AP_FOUND:
      LOG_WARN("no AP found");
      wifi_disconnected(wifi);
      break;

    case STATION_CONNECT_FAIL:
      LOG_WARN("connect failed");
      wifi_disconnected(wifi);
      break;

    case STATION_GOT_IP:
      LOG_INFO("got IP");
      wifi_connected(wifi);
      break;

    default:
      // https://github.com/espressif/ESP8266_NONOS_SDK/issues/153 returns 0xff if not configured?
      if (status == 0xff) {
        LOG_WARN("not configured?");
      } else {
        LOG_WARN("Unknown wifi_station_get_connect_status -> %d", status);
      }
      break;
  }
}

int wifi_setup(struct wifi *wifi, const struct wifi_config *config)
{
  struct station_config wifi_station_config = { };

  snprintf((char *) wifi_station_config.ssid, sizeof(wifi_station_config.ssid), "%s", config->ssid);
  snprintf((char *) wifi_station_config.password, sizeof(wifi_station_config.password), "%s", config->password);

  if (!wifi_set_opmode(STATION_MODE)) {
    LOG_ERROR("wifi_set_opmode STATION_MODE");
    return -1;
  }

  if (wifi_station_config.ssid[0]) {
    LOG_INFO("config station mode with ssid=%s", wifi_station_config.ssid);

    if (!wifi_station_set_config_current(&wifi_station_config)) {
      LOG_ERROR("wifi_station_set_config_current");
      return -1;
    }

    wifi_update_status(wifi, STATION_CONNECTING);

  } else {
    LOG_INFO("no wifi ssid configured");

    wifi_update_status(wifi, STATION_IDLE);
  }

  return 0;
}

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
      LOG_WARN("unknown event=%u\n", event->event_id);
    }
  }

  wifi_update_status(&wifi, wifi_station_get_connect_status());
}

int init_wifi(struct wifi_config *config, struct user_info *user_info, user_func_t user_func)
{
  int err;

  if ((err = wifi_init(&wifi, user_info, user_func))) {
    return err;
  }

  if ((err = wifi_setup(&wifi, config))) {
    return err;
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
