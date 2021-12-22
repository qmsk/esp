#pragma once

#include <artnet.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define DMX_OUTPUT_COUNT 2

/* dmx_config.c */
struct dmx_config {
  bool enabled;

  int uart;

  bool input_enabled;
  bool input_artnet_enabled;
  uint16_t input_artnet_universe;
};

struct dmx_output_config {
  bool enabled;

  uint16_t gpio_pin;
  int gpio_mode;

  bool artnet_enabled;
  uint16_t artnet_universe;
};

extern struct dmx_config dmx_config;
extern struct dmx_output_config dmx_output_configs[DMX_OUTPUT_COUNT];

/* dmx.c */
extern struct uart *dmx_uart;

/* dmx_input.c */
struct dmx_input_state {
  struct dmx_input *dmx_input;

  struct artnet_input *artnet_input;
  struct artnet_dmx artnet_dmx;
};

extern struct dmx_input_state *dmx_input_state;

int init_dmx_inputs();

int dmx_input_main(struct dmx_input_state *state);

/* dmx_output.c */
struct dmx_output_state {
  struct dmx_output *dmx_output;

  xTaskHandle task;

  xQueueHandle artnet_queue;
  struct artnet_dmx artnet_dmx;
};

extern struct dmx_output_state dmx_output_states[DMX_OUTPUT_COUNT];

int init_dmx_outputs();
int start_dmx_outputs();

int output_dmx(struct dmx_output_state *state, void *data, size_t len);
