#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

struct artnet_config {
  bool enabled;
  uint16_t net, subnet;
};

extern struct artnet_config artnet_config;
extern struct artnet *artnet;

/*
 * universe: 0-15 to be combined with artnet net/subnet
 */
int add_artnet_output(uint16_t universe, xQueueHandle queue);

/*
 * universe: 0-15 to be combined with artnet net/subnet
 */
int add_artnet_outputs(uint16_t universe, uint8_t index, xQueueHandle queue, xTaskHandle task);

/*
 * reconfigure artnet receiver
 */
int update_artnet();
