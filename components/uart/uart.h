#pragma once

#include <uart.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/stream_buffer.h>
#include <freertos/task.h>

#if CONFIG_IDF_TARGET_ESP8266
# include <esp8266/uart_struct.h>
#elif CONFIG_IDF_TARGET_ESP32
# include <esp_intr_alloc.h>
# include <soc/uart_struct.h>
#else
# error Unsupported target
#endif

struct uart {
  uart_port_t port;
  uart_dev_t *dev;
#if CONFIG_IDF_TARGET_ESP32
  portMUX_TYPE mux;
  intr_handle_t intr;
#endif

  /*  */
  SemaphoreHandle_t dev_mutex, pin_mutex;

  /* RX */
  SemaphoreHandle_t rx_mutex;
  StreamBufferHandle_t rx_buffer;
  bool rx_overflow, rx_break, rx_error, rx_abort;

  /* TX */
  SemaphoreHandle_t tx_mutex;
  StreamBufferHandle_t tx_buffer;
#if CONFIG_IDF_TARGET_ESP8266
  TaskHandle_t txfifo_empty_notify_task;
#elif CONFIG_IDF_TARGET_ESP32
  TaskHandle_t tx_done_notify_task;
#endif
};

/* pin.c */
int uart_pin_setup(struct uart *uart, struct uart_options options);

/* give pin_mutex */
void uart_pin_teardown(struct uart *uart);

/* dev.c */

/* take dev_mutex, set dev */
int uart_dev_setup(struct uart *uart, struct uart_options options);

/* give dev_mutex, clear dev */
void uart_dev_teardown(struct uart *uart);

/* rx.c */
enum uart_rx_event {
  UART_RX_NONE = 0,
  UART_RX_DATA,
  UART_RX_OVERFLOW,
  UART_RX_ERROR,
  UART_RX_BREAK,
  UART_RX_BREAK_OVERFLOW,
  UART_RX_ABORT,
  UART_RX_DISABLED,
};

int uart_rx_init(struct uart *uart, size_t rx_buffer_size);
int uart_rx_setup(struct uart *uart, struct uart_options options);
enum uart_rx_event uart_rx_event(struct uart *uart);
int uart_rx_read(struct uart *uart, void *buf, size_t size, TickType_t timeout);
void uart_rx_abort(struct uart *uart);

/* tx.c */
int uart_tx_init(struct uart *uart, size_t tx_buffer_size);
int uart_tx_setup(struct uart *uart, struct uart_options options);

int uart_tx_one(struct uart *uart, uint8_t byte, TickType_t timeout);

size_t uart_tx_fast(struct uart *uart, const uint8_t *buf, size_t len);
size_t uart_tx_slow(struct uart *uart, const uint8_t *buf, size_t len, TickType_t timeout);

int uart_tx_flush(struct uart *uart);

int uart_tx_break(struct uart *uart, unsigned bits);
int uart_tx_mark(struct uart *uart, unsigned bits);

/* intr.c */
int uart_intr_setup(struct uart *uart);
void uart_intr_teardown(struct uart *uart);
