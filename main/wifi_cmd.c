#include "wifi.h"
#include "wifi_internal.h"

#include <logging.h>

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
    config.sta.threshold.authmode = WIFI_AUTHMODE_THRESHOLD;
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

  if ((err = tcpip_adapter_info(TCPIP_ADAPTER_IF_STA))) {
    LOG_ERROR("tcpip_adapter_info");
    return err;
  }

  if ((err = tcpip_adapter_info(TCPIP_ADAPTER_IF_AP))) {
    LOG_ERROR("tcpip_adapter_info");
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
