#pragma once

#include <artnet.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

int add_artnet_output(struct artnet_output_options options, xQueueHandle queue);
