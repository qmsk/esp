#pragma once

#include <config.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

extern const struct configtab artnet_configtab[];

int init_artnet();

/*
 * universe: 0-15 combined with net/subnet
 */
int add_artnet_output(uint16_t universe, xQueueHandle queue);

int start_artnet();
