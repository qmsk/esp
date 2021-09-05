#ifndef DMX_H
#define DMX_H

#include <gpio_out.h>
#include <uart1.h>

#define DMX_BAUD_RATE UART1_BAUD_250000
#define DMX_DATA_BITS UART1_DATA_BITS_8
#define DMX_PARTITY_BITS UART1_PARTIY_DISABLE
#define DMX_STOP_BITS UART1_STOP_BITS_2

// fit one complete DMX frame into the uart1 TX buffer
#define DMX_TX_BUFFER_SIZE (512 + 1)

#define DMX_BREAK_US 92
#define DMX_MARK_US 12

enum dmx_cmd {
    DMX_CMD_DIMMER = 0x00,
};

struct dmx_output;
struct dmx_output_options {
  struct gpio_out *gpio_out;
  enum gpio_out_pins gpio_out_pins;
};

int dmx_output_new (struct dmx_output **outp, struct uart1 *uart1, struct dmx_output_options options);

int dmx_output_enable (struct dmx_output *out);
int dmx_output_cmd (struct dmx_output *out, enum dmx_cmd cmd, void *data, size_t len);

#endif
