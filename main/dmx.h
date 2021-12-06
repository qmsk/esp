#pragma once

#include "dmx_config.h"

#include <artnet.h>
#include <cmd.h>
#include <config.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

/* dmx_config.c */
struct dmx_input_config {
  bool enabled;

  bool artnet_enabled;
  uint16_t artnet_universe;
};

struct dmx_output_config {
  bool enabled;

  uint16_t gpio_pin;
  int gpio_mode;

  bool artnet_enabled;
  uint16_t artnet_universe;
};

extern struct dmx_input_config dmx_input_config;
extern struct dmx_output_config dmx_output_configs[DMX_OUTPUT_COUNT];

/* dmx_input.c */
struct dmx_input_state {
  struct dmx_input *dmx_input;

  xTaskHandle task;

  struct artnet_input *artnet_input;
  struct artnet_dmx artnet_dmx;
};

/* dmx_output.c */
struct dmx_output_state {
  struct dmx_output *dmx_output;

  struct dmx_artnet_output {
    xTaskHandle task;
    xQueueHandle queue;

    struct artnet_dmx dmx;
  } artnet;
};

extern struct dmx_input_state *dmx_input_state;
extern struct dmx_output_state dmx_output_states[DMX_OUTPUT_COUNT];

int output_dmx(struct dmx_output_state *state, void *data, size_t len);

/* dmx_artnet.c */
int init_dmx_artnet_output(struct dmx_output_state *state, int index, const struct dmx_output_config *config);

/* dmx_cmd.c */
extern const struct cmdtab dmx_cmdtab;
