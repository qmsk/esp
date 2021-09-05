#pragma once

#include "dmx_config.h"

#include <artnet.h>
#include <cmd.h>
#include <config.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

/* dmx_config.c */
struct dmx_config {
  bool enabled;

  uint16_t gpio_pin;
  int gpio_mode;

  bool artnet_enabled;
  uint16_t artnet_universe;
};

extern struct dmx_config dmx_configs[DMX_COUNT];

/* dmx.c */
struct dmx_state {
  struct dmx_output *dmx_output;

  struct dmx_artnet {
    xTaskHandle task;
    xQueueHandle queue;

    struct artnet_dmx artnet_dmx;
  } artnet;
};

extern struct dmx_state dmx_states[DMX_COUNT];

int output_dmx(struct dmx_state *state, void *data, size_t len);

/* dmx_artnet.c */
int init_dmx_artnet(struct dmx_state *state, int index, const struct dmx_config *config);

/* dmx_cmd.c */
extern const struct cmdtab dmx_cmdtab;
