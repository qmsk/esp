#pragma once

#include <artnet.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct artnet_config {
  bool enabled;

  uint16_t net, subnet;
};

extern struct artnet_config artnet_config;
extern struct artnet *artnet;

/*
 * options.address: univers: 0-15 to be combined with artnet net/subnet
 */
 int add_artnet_input(struct artnet_input **inputp, struct artnet_input_options options);

/*
 * options.address: univers: 0-15 to be combined with artnet net/subnet
 */
int add_artnet_output(struct artnet_output **outputp, struct artnet_output_options options);

/*
 * reconfigure artnet receiver
 */
int update_artnet();

/*
 * Test artnet outputs
 */
void test_artnet();
void cancel_artnet_test();
