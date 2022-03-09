#pragma once

#include "dmx.h"

#include <dmx_input.h>
#include <dmx_output.h>
#include <uart.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct dmx_output_state;
struct dmx_output_config;

/* dmx_uart.c */
int init_dmx_uart();

int start_dmx_uart();

int open_dmx_input_uart(struct dmx_input *input);
int open_dmx_output_uart(struct dmx_output *output);

bool query_dmx_uart0();

void stop_dmx_uart();

/* dmx_gpio.c */
int init_dmx_gpio();

int config_dmx_output_gpio(struct dmx_output_state *state, const struct dmx_output_config *config, struct dmx_output_options *options);

/* dmx_input.c */
struct dmx_input_state {
  struct dmx_input *dmx_input;

  struct artnet_input *artnet_input;
  struct artnet_dmx *artnet_dmx;
};

extern struct dmx_input_state dmx_input_state;

int init_dmx_input();

int run_dmx_input(struct dmx_input_state *state);

int stop_dmx_input(struct dmx_input_state *state);

/* dmx_output.c */
struct dmx_output_state {
  int index;

  struct dmx_output *dmx_output;

  xTaskHandle artnet_task;
  struct artnet_dmx *artnet_dmx;
  struct artnet_output *artnet_output;
};

extern struct dmx_output_state dmx_output_states[DMX_OUTPUT_COUNT];

int init_dmx_outputs();

int start_dmx_outputs();

int output_dmx(struct dmx_output_state *state, void *data, size_t len);
