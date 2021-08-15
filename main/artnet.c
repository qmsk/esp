#include <artnet.h>
#include "artnet.h"

#include <logging.h>

#include <esp_err.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <tcpip_adapter.h>

#include <string.h>

#define ARTNET_TASK_NAME "artnet"
#define ARTNET_TASK_STACK 2048
#define ARTNET_TASK_PRIORITY tskIDLE_PRIORITY + 2

static const tcpip_adapter_if_t artnet_tcpip_adapter_if = TCPIP_ADAPTER_IF_STA;
static const wifi_interface_t artnet_wifi_interface = ESP_IF_WIFI_STA;

static const char artnet_product[] = "https://github.com/SpComb/esp-projects";

struct artnet_config {
  bool enabled;
  uint16_t net, subnet;
};

struct artnet_config artnet_config = {

};
struct artnet *artnet;

const struct configtab artnet_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &artnet_config.enabled },
  },
  { CONFIG_TYPE_UINT16, "net",
    .description = "Set network address, 0-127.",
    .uint16_type = { .value = &artnet_config.net, .max = ARTNET_NET_MAX },
  },
  { CONFIG_TYPE_UINT16, "subnet",
    .description = "Set sub-net address, 0-16.",
    .uint16_type = { .value = &artnet_config.subnet, .max = ARTNET_SUBNET_MAX },
  },
  {}
};

static int init_artnet_options(struct artnet_options *options, const struct artnet_config *config)
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
  options->address = artnet_address(config->net, config->subnet, 0);

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

  if ((err = init_artnet_options(&options, &artnet_config))) {
    LOG_ERROR("init_artnet_options");
    return err;
  }

  if ((err = artnet_new(&artnet, options))) {
    LOG_ERROR("artnet_new");
    return err;
  }

  return 0;
}

int add_artnet_output(uint16_t universe, xQueueHandle queue)
{
  const struct artnet_config *config = &artnet_config;

  uint16_t address = artnet_address(config->net, config->subnet, universe);

  if (artnet) {
    return artnet_add_output(artnet, address, queue);
  } else {
    return 0;
  }
}

// task
xTaskHandle _artnet_task;

static void artnet_task(void *ctx)
{
  struct artnet *artnet = ctx;
  int err;

  if ((err = artnet_main(artnet))) {
    LOG_ERROR("artnet_main");
  }
}

int start_artnet()
{
  int err;

  if (!artnet) {
    return 0;
  }

  if ((err = xTaskCreate(&artnet_task, ARTNET_TASK_NAME, ARTNET_TASK_STACK, artnet, ARTNET_TASK_PRIORITY, &_artnet_task)) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_DEBUG("artnet task=%p", _artnet_task);
  }

  return 0;
}
