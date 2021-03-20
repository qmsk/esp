#pragma once

#include <config.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

extern const struct configtab artnet_configtab[];

int init_artnet();

int add_artnet_output(uint16_t addr, xQueueHandle queue);

int start_artnet();
