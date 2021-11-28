#pragma once

#include <uart0.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/stream_buffer.h>
#include <freertos/task.h>

struct uart0 {
  SemaphoreHandle_t mutex;

  StreamBufferHandle_t rx_buffer;
  bool rx_overflow, rx_break, rx_error;
};

/* setup.c */
void uart0_setup(struct uart0_options options);

/* rx.c */
int uart0_rx_init(struct uart0 *uart0, size_t rx_buffer_size);

void uart0_rx_setup(struct uart0 *uart0);

int uart0_rx_read(struct uart0 *uart0, void *buf, size_t size);

/* intr.c */
int uart0_intr_init(struct uart0 *uart0);
void uart0_intr_setup(struct uart0_options options);
