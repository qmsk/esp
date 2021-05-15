#include "uart1.h"

#include <logging.h>

#include <driver/uart.h>
#include <esp_err.h>

#include <esp8266/uart_struct.h>
#include <esp8266/uart_register.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define UART1_INTR_MASK 0x1ff

void uart1_intr_handler(void *ctx)
{
  struct uart1 *uart = ctx;
  uint32_t int_st = uart1.int_st.val;
  BaseType_t task_woken = pdFALSE;

  if (int_st & UART_TXFIFO_EMPTY_INT_ST) {
    uart1_tx_intr_handler(uart, &task_woken);
  }

  if (task_woken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

void uart1_intr_setup()
{
  taskENTER_CRITICAL();

  // clear all interrupts
  uart1.int_clr.val = UART1_INTR_MASK;

  // clear enable mask
  uart1.int_ena.val = 0;

  taskEXIT_CRITICAL();
}

int uart1_intr_start(struct uart1 *uart1)
{
  esp_err_t err;

  // use standard uart driver ISR for uart0/1 muxing
  if ((err = uart_isr_register(UART_NUM_1, uart1_intr_handler, uart1))) {
    LOG_ERROR("uart_isr_register: %s", esp_err_to_name(err));
  }

  return 0;
}
