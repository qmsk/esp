#include <artnet.h>
#include "artnet.h"
#include "artnet_state.h"
//#include "dmx_artnet.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <string.h>

#define ARTNET_PRODUCT "https://github.com/SpComb/esp-projects"

#define ARTNET_INPUTS 4

#define ARTNET_LISTEN_TASK_NAME "artnet-listen"
#define ARTNET_INPUTS_TASK_NAME "artnet-inputs"
#define ARTNET_TASK_STACK 2048
#define ARTNET_TASK_PRIORITY tskIDLE_PRIORITY + 2

struct artnet *artnet;

unsigned count_artnet_inputs()
{
  unsigned inputs = 0;

  // TODO: inputs += count_dmx_artnet_inputs();

  return inputs;
}

static int init_artnet_options(struct artnet_options *options, const struct artnet_config *config)
{
  // uint8_t mac[6] = {};
  // const char *hostname = NULL;
  // ip4_addr_t ip_addr = {};
  // int err;

/* TODO
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
*/

  options->port = ARTNET_PORT;
  options->address = artnet_address(config->net, config->subnet, 0);

/* TODO
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
*/

  // TODO: snprintf(options->short_name, sizeof(options->short_name), "%s", hostname ? hostname : "");
  snprintf(options->long_name, sizeof(options->long_name), "%s", ARTNET_PRODUCT);

  options->inputs = count_artnet_inputs();

  LOG_INFO("port=%u address=%04x inputs=%u",
    options->port,
    options->address,
    options->inputs
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

int add_artnet_input(struct artnet_input **inputp, struct artnet_input_options options)
{
  const struct artnet_config *config = &artnet_config;

  if (!artnet) {
    LOG_ERROR("artnet disabled");
    return -1;
  }

  options.address = artnet_address(config->net, config->subnet, options.address);

  return artnet_add_input(artnet, inputp, options);
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
xTaskHandle artnet_listen_task, artnet_inputs_task;

static void artnet_main_listen(void *ctx)
{
  struct artnet *artnet = ctx;
  int err;

  if ((err = artnet_listen_main(artnet))) {
    LOG_ERROR("artnet_listen_main");
  }
}

static void artnet_main_inputs(void *ctx)
{
  struct artnet *artnet = ctx;
  int err;

  if ((err = artnet_inputs_main(artnet))) {
    LOG_ERROR("artnet_inputs_main");
  }
}

int start_artnet()
{
  int err;

  if (!artnet) {
    return 0;
  }

  if ((err = xTaskCreate(&artnet_main_listen, ARTNET_LISTEN_TASK_NAME, ARTNET_TASK_STACK, artnet, ARTNET_TASK_PRIORITY, &artnet_listen_task)) <= 0) {
    LOG_ERROR("xTaskCreate(artnet-listen)");
    return -1;
  } else {
    LOG_DEBUG("artnet listen task=%p", artnet_listen_task);
  }

  if (artnet_get_inputs_enabled(artnet)) {
    if ((err = xTaskCreate(&artnet_main_inputs, ARTNET_INPUTS_TASK_NAME, ARTNET_TASK_STACK, artnet, ARTNET_TASK_PRIORITY, &artnet_inputs_task)) <= 0) {
      LOG_ERROR("xTaskCreate(artnet-inputs)");
      return -1;
    } else {
      LOG_DEBUG("artnet inputs task=%p", artnet_inputs_task);
    }
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

void test_artnet()
{
  int err;

  if (!artnet) {
    return;
  }

  if ((err = artnet_test_outputs(artnet))) {
    LOG_ERROR("artnet_test_outputs");
  }
}

void cancel_artnet_test()
{
  int err;

  if (!artnet) {
    return;
  }

  if ((err = artnet_sync_outputs(artnet))) {
    LOG_ERROR("artnet_sync_outputs");
  }
}
