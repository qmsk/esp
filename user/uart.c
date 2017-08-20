#include <drivers/uart.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <stdio.h>

#include "uart.h"
#include "user_config.h"
#include "logging.h"

#define UART_RX_STACK 256
#define UART_RX_QUEUE 8

struct uart_rx_event {
  char buf[32];
  size_t len;
};

struct uart {
  xTaskHandle rx_task;
  xQueueHandle rx_queue;

  char rx_buf[1024], *rx_ptr;
  char tx_buf[1024];
  size_t tx_off;
} uart;

void uart_flush()
{
  // TODO: blocking?
  UART_Write(UART0, uart.tx_buf, uart.tx_off);

  uart.tx_off = 0;
}

int uart_vprintf(const char *fmt, va_list vargs)
{
  char *tx_ptr = uart.tx_buf + uart.tx_off;
  size_t tx_size = sizeof(uart.tx_buf) - uart.tx_off;
  int ret;

  if ((ret = vsnprintf(tx_ptr, tx_size, fmt, vargs)) < 0) {
    // error
  } else if (ret < tx_size) {
    uart.tx_off += ret;
    uart_flush();
  } else {
    // overflow, write truncated...
    uart_flush();
  }

  return ret;
}

int uart_printf(const char *fmt, ...)
{
  int ret;
  va_list vargs;

  va_start(vargs, fmt);
  ret = uart_vprintf(fmt, vargs);
  va_end(vargs);

  return ret;
}

// uart->rx_buf contains a '\0'-terminated string
// it either ends with a '\n', or it fills the entire buffer
static void uart_rx_flush(struct uart *uart)
{
  LOG_DEBUG("%s", uart->rx_buf);

  uart->rx_ptr = uart->rx_buf;
}

static void uart_rx_event(struct uart *uart, const struct uart_rx_event *event)
{
  for (const char *in = event->buf; in < event->buf + event->len; in++) {
    *uart->rx_ptr++ = *in;

    if (*in == '\n' || uart->rx_ptr + 1 >= uart->rx_buf + sizeof(uart->rx_buf)) {
      *uart->rx_ptr = '\0';

      uart_rx_flush(uart);
    }
  }
}

static void uart_intr_handler(void *arg)
{
  uint32 intr_status = UART_GetIntrStatus(UART0);
  portBASE_TYPE task_woken;

  if (intr_status & (UART_RXFIFO_FULL_INT_ST | UART_RXFIFO_TOUT_INT_ST)) {
    struct uart_rx_event rx_event;

    rx_event.len = UART_Read(UART0, rx_event.buf, sizeof(rx_event.buf));

    if (xQueueSendFromISR(uart.rx_queue, &rx_event, &task_woken) <= 0) {
      // TODO: set overflow flag?
      os_printf("XXX uart_intr_handler: xQueueSendFromISR\n");
    }
  }

  UART_ClearIntrStatus(UART0, UART_RXFIFO_FULL_INT_ST | UART_RXFIFO_TOUT_INT_ST);

  portEND_SWITCHING_ISR(task_woken);
}

static void uart_rx_task(void *arg)
{
  struct uart *uart = arg;
  struct uart_rx_event rx_event;

  LOG_INFO("init uart=%p", uart);

  for (;;) {
    if (xQueueReceive(uart->rx_queue, &rx_event, portMAX_DELAY)) {
      if (rx_event.len < sizeof(rx_event.buf)) {
        rx_event.buf[rx_event.len] = '\0';
      }

      LOG_DEBUG("read len=%d: %.32s", rx_event.len, rx_event.buf);

      uart_rx_event(uart, &rx_event);

    } else {
      LOG_ERROR("xQueueReceive");
    }
  }
}

int init_uart(struct user_config *config)
{
  int err;

  UART_Config uart_config = {
    .baud_rate  = USER_CONFIG_UART_BAUD_RATE,
    .data_bits  = UART_WordLength_8b,
    .parity     = USART_Parity_None,
    .stop_bits  = USART_StopBits_1,
    .flow_ctrl  = USART_HardwareFlowControl_None,
    .flow_rx_thresh = 0,
    .inverse_mask = UART_None_Inverse,
  };
  UART_IntrConfig intr_config = {
    .enable_mask        = UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA,
    .rx_full_thresh     = 32, // bytes
    .rx_timeout_thresh  = 2, // time = 8-bits/baud-rate
  };

  uart.rx_ptr = uart.rx_buf;

  ETS_UART_INTR_DISABLE();

  UART_WaitTxFifoEmpty(UART0);
  UART_Setup(UART0, &uart_config);
  UART_SetupIntr(UART0, &intr_config);
  UART_RegisterIntrHandler(&uart_intr_handler, NULL);

  printf("INFO uart: setup baud=%u\n", uart_config.baud_rate);

  if ((uart.rx_queue = xQueueCreate(UART_RX_QUEUE, sizeof(struct uart_rx_event))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  if ((err = xTaskCreate(&uart_rx_task, (void *) "uart_rx", UART_RX_STACK, &uart, tskIDLE_PRIORITY + 2, &uart.rx_task)) <= 0) {
    LOG_ERROR("xTaskCreate uart_rx_task: %d", err);
    return -1;
  }

  ETS_UART_INTR_ENABLE();

  return 0;
}
