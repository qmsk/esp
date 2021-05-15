#pragma once

#include <uart1.h>

#include <freertos/FreeRTOS.h>
#include <freertos/stream_buffer.h>

struct uart1 {
  StreamBufferHandle_t tx_buffer;
};

// uart_tx
void uart1_tx_setup(struct uart1_options options);

size_t uart1_tx_raw(const uint8_t *buf, size_t size);
size_t uart1_tx_fast(struct uart1 *uart, const uint8_t *buf, size_t len);
size_t uart1_tx_slow(struct uart1 *uart, const uint8_t *buf, size_t len);

int uart1_tx(struct uart1 *uart, uint8_t byte);

void uart1_tx_intr_handler(struct uart1 *uart, BaseType_t *task_woken);

// uart intr
void uart1_intr_setup();

int uart1_intr_start(struct uart1 *uart1);
