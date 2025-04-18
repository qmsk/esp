#include <artnet.h>
#include "artnet.h"
#include "artnet_state.h"
#include "artnet_config.h"
#include "dmx_artnet.h"
#include "leds_artnet.h"
#include "system_network.h"
#include "tasks.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <string.h>

#define ARTNET_PRODUCT "https://github.com/qmsk/esp"

#define ARTNET_INPUTS 4

struct artnet *artnet;

unsigned count_artnet_inputs()
{
  unsigned inputs = 0;

  inputs += count_dmx_artnet_inputs();

  return inputs;
}

unsigned count_artnet_outputs()
{
  unsigned outputs = 0;

  outputs += count_dmx_artnet_outputs();
  outputs += count_leds_artnet_outputs();

  return outputs;
}

static int build_artnet_metadata(struct artnet_metadata *metadata)
{
  uint8_t mac[6] = {};
  const char *hostname = NULL;
  SYSTEM_IPV4_ADDR_TYPE ipv4_addr = {};
  int err;

  if ((err = get_system_mac(mac)) < 0) {
    LOG_ERROR("get_system_mac");
    return err;
  }

  if ((err = get_system_hostname(&hostname)) < 0) {
    LOG_ERROR("get_system_hostname");
    return err;
  }

  if ((err = get_system_ipv4_addr(&ipv4_addr)) < 0) {
    LOG_ERROR("get_system_ipv4_addr");
    return err;
  }

  metadata->ip_address[0] = SYSTEM_IPV4_ADDR_BYTE1(&ipv4_addr);
  metadata->ip_address[1] = SYSTEM_IPV4_ADDR_BYTE2(&ipv4_addr);
  metadata->ip_address[2] = SYSTEM_IPV4_ADDR_BYTE3(&ipv4_addr);
  metadata->ip_address[3] = SYSTEM_IPV4_ADDR_BYTE4(&ipv4_addr);

  metadata->mac_address[0] = mac[0];
  metadata->mac_address[1] = mac[1];
  metadata->mac_address[2] = mac[2];
  metadata->mac_address[3] = mac[3];
  metadata->mac_address[4] = mac[4];
  metadata->mac_address[5] = mac[5];

  snprintf(metadata->short_name, sizeof(metadata->short_name), "%s", hostname ? hostname : "");
  snprintf(metadata->long_name, sizeof(metadata->long_name), "%s", ARTNET_PRODUCT);

  return 0;
}

int init_artnet()
{
  const struct artnet_config *config = &artnet_config;
  struct artnet_options options = {
    .port     = ARTNET_UDP_PORT,
    .inputs   = count_artnet_inputs(),
    .outputs  = count_artnet_outputs(),
  };
  int err;

  if (!artnet_config.enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  if ((err = build_artnet_metadata(&options.metadata))) {
    LOG_ERROR("build_artnet_metadata");
    return err;
  }

  LOG_INFO("config net=%d subnet=%d", config->net, config->subnet);
  LOG_INFO("options port=%u inputs=%u outputs=%u",
    options.port,
    options.inputs,
    options.outputs
  );
  LOG_INFO("metadata ip_address=%u.%u.%u.%u", options.metadata.ip_address[0], options.metadata.ip_address[1], options.metadata.ip_address[2], options.metadata.ip_address[3]);
  LOG_INFO("metadata mac_address=%02x:%02x:%02x:%02x:%02x:%02x", options.metadata.mac_address[0], options.metadata.mac_address[1], options.metadata.mac_address[2], options.metadata.mac_address[3], options.metadata.mac_address[4], options.metadata.mac_address[5]);

  if ((err = artnet_new(&artnet, options))) {
    LOG_ERROR("artnet_new");
    return err;
  }

  return 0;
}

int add_artnet_input(struct artnet_input **inputp, struct artnet_input_options options)
{
  const struct artnet_config *config = &artnet_config;

  if (!artnet) {
    LOG_ERROR("artnet disabled");
    return -1;
  }

  options.address = artnet_address(config->net, config->subnet, options.address);

  LOG_INFO("address=%04x", options.address);

  return artnet_add_input(artnet, inputp, options);
}

int add_artnet_output(struct artnet_output **outputp, struct artnet_output_options options)
{
  const struct artnet_config *config = &artnet_config;

  if (!artnet) {
    LOG_ERROR("artnet disabled");
    return -1;
  }

  options.address = artnet_address(config->net, config->subnet, options.address);

  LOG_INFO("name=%s address=%04x", options.name, options.address);

  return artnet_add_output(artnet, outputp, options);
}

// task
xTaskHandle artnet_listen_task, artnet_inputs_task;

static void artnet_main_listen(void *ctx)
{
  struct artnet *artnet = ctx;
  int err;

  LOG_INFO("run network listen loop...");

  if ((err = artnet_listen_main(artnet))) {
    LOG_ERROR("artnet_listen_main");
  }
}

static void artnet_main_inputs(void *ctx)
{
  struct artnet *artnet = ctx;
  int err;

  LOG_INFO("run physical input loop...");

  if ((err = artnet_inputs_main(artnet))) {
    LOG_ERROR("artnet_inputs_main");
  }
}

int start_artnet()
{
  struct task_options listen_task_options = {
    .main       = artnet_main_listen,
    .name       = ARTNET_LISTEN_TASK_NAME,
    .stack_size = ARTNET_LISTEN_TASK_STACK,
    .arg        = artnet,
    .priority   = ARTNET_LISTEN_TASK_PRIORITY,
    .handle     = &artnet_listen_task,
    .affinity   = ARTNET_LISTEN_TASK_AFFINITY,
  };
  struct task_options input_task_options = {
    .main       = artnet_main_inputs,
    .name       = ARTNET_INPUTS_TASK_NAME,
    .stack_size = ARTNET_INPUTS_TASK_STACK,
    .arg        = artnet,
    .priority   = ARTNET_INPUTS_TASK_PRIORITY,
    .handle     = &artnet_inputs_task,
    .affinity   = ARTNET_INPUTS_TASK_AFFINITY,
  };
  int err;

  if (!artnet) {
    return 0;
  }

  if ((err = start_task(listen_task_options))) {
    LOG_ERROR("start_task(artnet-listen)");
    return -1;
  } else {
    LOG_DEBUG("artnet listen task=%p", artnet_listen_task);
  }

  if (!artnet_get_inputs_enabled(artnet)) {
    LOG_DEBUG("no artnet inputs configured");
  } else if ((err = start_task(input_task_options))) {
    LOG_ERROR("start_task(artnet-inputs)");
    return -1;
  } else {
    LOG_DEBUG("artnet inputs task=%p", artnet_inputs_task);
  }

  return 0;
}

int update_artnet_network()
{
  struct artnet_metadata metadata = {};
  int err;

  if (!artnet) {
    return 0;
  }

  if ((err = build_artnet_metadata(&metadata))) {
    LOG_ERROR("build_artnet_metadata");
    return err;
  }

  LOG_INFO("re-initialize Art-NET metadata:");
  LOG_INFO("\tip_address=%u.%u.%u.%u", metadata.ip_address[0], metadata.ip_address[1], metadata.ip_address[2], metadata.ip_address[3]);
  LOG_INFO("\tmac_address=%02x:%02x:%02x:%02x:%02x:%02x", metadata.mac_address[0], metadata.mac_address[1], metadata.mac_address[2], metadata.mac_address[3], metadata.mac_address[4], metadata.mac_address[5]);
  LOG_INFO("\tshort_name=%s", metadata.short_name);

  if ((err = artnet_set_metadata(artnet, &metadata))) {
    LOG_ERROR("artnet_set_metadata");
    return err;
  }

  return 0;
}
