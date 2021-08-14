#include <artnet.h>
#include "artnet.h"

#include <logging.h>

#include <esp_err.h>
#include <tcpip_adapter.h>
#include <esp_wifi.h>

#include <string.h>

static const tcpip_adapter_if_t artnet_tcpip_adapter_if = TCPIP_ADAPTER_IF_STA;
static const wifi_interface_t artnet_wifi_interface = ESP_IF_WIFI_STA;

static const char artnet_product[] = "https://github.com/SpComb/esp-projects";

#define ARTNET_CONFIG_UNIVERSE 0

struct artnet_config {
  bool enabled;
  uint16_t universe;
};

struct artnet_config artnet_config = {
  .universe = ARTNET_CONFIG_UNIVERSE,
};
struct artnet *artnet;

const struct configtab artnet_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &artnet_config.enabled },
  },
  { CONFIG_TYPE_UINT16, "universe",
    .uint16_type = { .value = &artnet_config.universe },
  },
  {}
};

static int init_artnet_options(struct artnet_options *options)
{
  const char *hostname;
  tcpip_adapter_ip_info_t ip_info;
  uint8_t mac[6];
  esp_err_t err;

  if ((err = tcpip_adapter_get_hostname(artnet_tcpip_adapter_if, &hostname))) {
    LOG_ERROR("tcpip_adapter_get_hostname: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = tcpip_adapter_get_ip_info(artnet_tcpip_adapter_if, &ip_info))) {
    LOG_ERROR("tcpip_adapter_get_ip_info: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_get_mac(artnet_wifi_interface, mac))) {
    LOG_ERROR("esp_wifi_get_mac: %s", esp_err_to_name(err));
    return -1;
  }

  options->port = ARTNET_PORT;
  options->universe = artnet_config.universe;

  options->ip_address[0] = ip4_addr1(&ip_info.ip);
  options->ip_address[1] = ip4_addr2(&ip_info.ip);
  options->ip_address[2] = ip4_addr3(&ip_info.ip);
  options->ip_address[3] = ip4_addr4(&ip_info.ip);

  options->mac_address[0] = mac[0];
  options->mac_address[1] = mac[1];
  options->mac_address[2] = mac[2];
  options->mac_address[3] = mac[3];
  options->mac_address[4] = mac[4];
  options->mac_address[5] = mac[5];

  snprintf(options->short_name, sizeof(options->short_name), "%s", hostname);
  snprintf(options->long_name, sizeof(options->long_name), "%s", artnet_product);

  return 0;
}

int init_artnet()
{
  struct artnet_options options = {};
  int err;

  if (!artnet_config.enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  LOG_INFO("enabled: universe=%u", artnet_config.universe);

  if ((err = init_artnet_options(&options))) {
    LOG_ERROR("init_artnet_options");
    return err;
  }

  if ((err = artnet_new(&artnet, options))) {
    LOG_ERROR("artnet_new");
    return err;
  }

  return 0;
}

int add_artnet_output(uint16_t addr, xQueueHandle queue)
{
  if (artnet) {
    return artnet_add_output(artnet, addr, queue);
  } else {
    return 0;
  }
}

int start_artnet()
{
  if (artnet) {
    return artnet_start(artnet);
  } else {
    return 0;
  }
}
