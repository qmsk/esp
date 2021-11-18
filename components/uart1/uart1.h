#pragma once

#include <uart1.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/stream_buffer.h>
#include <freertos/task.h>

struct uart1 {
  SemaphoreHandle_t mutex;

  StreamBufferHandle_t tx_buffer;
  TaskHandle_t txfifo_empty_notify_task;
};

// uart_tx
void uart1_tx_setup(struct uart1_options options);

int uart1_tx_one(struct uart1 *uart, uint8_t byte);

size_t uart1_tx_fast(struct uart1 *uart, const uint8_t *buf, size_t len);
size_t uart1_tx_buf(struct uart1 *uart, const uint8_t *buf, size_t len);
size_t uart1_tx_slow(struct uart1 *uart, const uint8_t *buf, size_t len);

int uart1_tx_flush(struct uart1 *uart);

void uart1_tx_break(struct uart1 *uart);
void uart1_tx_mark(struct uart1 *uart);

void uart1_tx_intr_handler(struct uart1 *uart, BaseType_t *task_woken);

// uart intr
void uart1_intr_setup();

int uart1_intr_start(struct uart1 *uart1);
