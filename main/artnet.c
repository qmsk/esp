#include <artnet.h>
#include "artnet.h"
#include "artnet_config.h"
#include "artnet_init.h"
#include "system.h"

#include <logging.h>

#include <esp_err.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <tcpip_adapter.h>

#include <string.h>

#define ARTNET_PRODUCT "https://github.com/SpComb/esp-projects"

#define ARTNET_TASK_NAME "artnet"
#define ARTNET_TASK_STACK 2048
#define ARTNET_TASK_PRIORITY tskIDLE_PRIORITY + 2

struct artnet *artnet;

static int init_artnet_options(struct artnet_options *options, const struct artnet_config *config)
{
  uint8_t mac[6] = {};
  const char *hostname = NULL;
  ip4_addr_t ip_addr = {};
  int err;

  if ((err = get_system_mac(mac)) < 0) {
    LOG_ERROR("get_system_mac");
    return err;
  }

  if ((err = get_system_hostname(&hostname)) < 0) {
    LOG_ERROR("get_system_hostname");
    return err;
  }

  if ((err = get_system_ipv4_addr(&ip_addr)) < 0) {
    LOG_ERROR("get_system_ipv4_addr");
    return err;
  }

  options->port = ARTNET_PORT;
  options->address = artnet_address(config->net, config->subnet, 0);

  options->ip_address[0] = ip4_addr1(&ip_addr);
  options->ip_address[1] = ip4_addr2(&ip_addr);
  options->ip_address[2] = ip4_addr3(&ip_addr);
  options->ip_address[3] = ip4_addr4(&ip_addr);

  options->mac_address[0] = mac[0];
  options->mac_address[1] = mac[1];
  options->mac_address[2] = mac[2];
  options->mac_address[3] = mac[3];
  options->mac_address[4] = mac[4];
  options->mac_address[5] = mac[5];

  snprintf(options->short_name, sizeof(options->short_name), "%s", hostname ? hostname : "");
  snprintf(options->long_name, sizeof(options->long_name), "%s", ARTNET_PRODUCT);

  LOG_INFO("port=%u address=%04x",
    options->port,
    options->address
  );
  LOG_INFO("ip_address=%u.%u.%u.%u",
    options->ip_address[0],
    options->ip_address[1],
    options->ip_address[2],
    options->ip_address[3]
  );
  LOG_INFO("mac_address=%02x:%02x:%02x:%02x:%02x:%02x",
    options->mac_address[0],
    options->mac_address[1],
    options->mac_address[2],
    options->mac_address[3],
    options->mac_address[4],
    options->mac_address[5]
  );

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

int add_artnet_output(struct artnet_output_options options, xQueueHandle queue)
{
  const struct artnet_config *config = &artnet_config;

  if (!artnet) {
    LOG_ERROR("artnet disabled");
    return -1;
  }

  options.address = artnet_address(config->net, config->subnet, options.address);

  return artnet_add_output(artnet, options, queue);
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

int update_artnet()
{
  struct artnet_options options = {};
  int err;

  if (!artnet) {
    return 0;
  }

  LOG_INFO("re-initialize artnet discovery options");

  if ((err = init_artnet_options(&options, &artnet_config))) {
    LOG_ERROR("init_artnet_options");
    return err;
  }

  if ((err = artnet_set_options(artnet, options))) {
    LOG_ERROR("artnet_set_options");
    return err;
  }

  return 0;
}
