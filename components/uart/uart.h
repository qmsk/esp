#pragma once

#include <uart.h>

#include <esp8266/uart_struct.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/stream_buffer.h>
#include <freertos/task.h>

struct uart {
  enum uart_port port;
  uart_dev_t *dev;
  SemaphoreHandle_t pin_mutex;

  /* RX */
  SemaphoreHandle_t rx_mutex;
  StreamBufferHandle_t rx_buffer;
  bool rx_overflow, rx_break, rx_error;

  TickType_t read_timeout;

  /* TX */
  SemaphoreHandle_t tx_mutex;
  StreamBufferHandle_t tx_buffer;
  TaskHandle_t txfifo_empty_notify_task;
};

/* dev.c */
int uart_dev_init(struct uart *uart);
int uart_dev_setup(struct uart *uart, struct uart_options options);
void uart_dev_teardown(struct uart *uart);

/* rx.c */
enum uart_rx_event {
  UART_RX_NONE = 0,
  UART_RX_DATA,
  UART_RX_OVERFLOW,
  UART_RX_ERROR,
  UART_RX_BREAK,
};

int uart_rx_init(struct uart *uart, size_t rx_buffer_size);
void uart_rx_setup(struct uart *uart, struct uart_options options);
enum uart_rx_event uart_rx_event(struct uart *uart);
int uart_rx_read(struct uart *uart, void *buf, size_t size, TickType_t timeout);

/* tx.c */
int uart_tx_init(struct uart *uart, size_t tx_buffer_size);
void uart_tx_reset(struct uart *uart, struct uart_options options);

int uart_tx_one(struct uart *uart, uint8_t byte);

size_t uart_tx_fast(struct uart *uart, const uint8_t *buf, size_t len);
size_t uart_tx_buffered(struct uart *uart, const uint8_t *buf, size_t len);
size_t uart_tx_slow(struct uart *uart, const uint8_t *buf, size_t len);

int uart_tx_flush(struct uart *uart);

void uart_tx_break(struct uart *uart);
void uart_tx_mark(struct uart *uart);

/* intr.c */
void uart_intr_setup(struct uart *uart);
void uart_intr_teardown(struct uart *uart);

/* isr.c */
typedef void (* uart_intr_func_t)(void *arg);

void uart_isr_init();
void uart_isr_setup(enum uart_port port, uart_intr_func_t func, void *arg);
void uart_isr_teardown(enum uart_port port);
