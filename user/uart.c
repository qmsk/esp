#include "uart.h"

#include <drivers/uart.h>
#include <freertos/task.h>

#include "user_config.h"
#include "logging.h"

#define UART_TX_BUFFER 1024

struct uart {
  xQueueHandle tx_queue;

  xQueueHandle rx_queue;
  bool rx_overflow;
} uart;

// drain TX queue by writing to UART FIFO
// Leaves the TXFIFO_EMPTY interrupt enabled if the TX buffer still has data
static void uart_intr_tx(portBASE_TYPE *task_wokenp) // ISR UART_TXFIFO_EMPTY_INT_ST
{
  struct uart_event tx_event;

  while (true) {
    if (UART_GetWriteSize(UART0) < sizeof(tx_event.buf)) {
      // not enough room in the UART TX fifo to write out a complete event
      break;
    }

    if (!xQueueReceiveFromISR(uart.tx_queue, &tx_event, task_wokenp)) {
      // queue empty
      UART_DisableIntr(UART0, UART_TXFIFO_EMPTY_INT_ENA);

      break;
    }

    uint8_t *buf = tx_event.buf;
    size_t len = tx_event.len;
    size_t write = 0;

    do {
      // this should happen in a single write assuming the UART_GetWriteSize() check
      write += UART_Write(UART0, buf + write, len - write);
    } while (write < len);
  }
}

static void uart_intr_rx(portBASE_TYPE *task_wokenp) // ISR UART_RXFIFO_FULL_INT_ST | UART_RXFIFO_TOUT_INT_ST
{
  struct uart_event rx_event = {.flags = 0};

  if (uart.rx_overflow) {
    rx_event.flags |= UART_RX_OVERFLOW;
  }

  // must read to clear the interrupt, even if queue overflow
  rx_event.len = UART_Read(UART0, rx_event.buf, sizeof(rx_event.buf));

  if (!uart.rx_queue) {
    uart.rx_overflow = true;
  } else if (xQueueSendFromISR(uart.rx_queue, &rx_event, task_wokenp) <= 0) {
    uart.rx_overflow = true;
  } else {
    uart.rx_overflow = false;
  }
}

static void uart_intr_rx_overflow() // ISR UART_RXFIFO_OVF_INT_ST
{
  // lost data after last read
  uart.rx_overflow = true;
}

// Push TX event to queue, enabling the TX interrupt
// Blocks if the queue is full
// @return <0 on timeout, 0 on success
int uart_tx(const struct uart_event *tx_event)
{
  if (xQueueSend(uart.tx_queue, tx_event, portMAX_DELAY)) {
    // XXX: can't enable this before sending, it the ISR will race and disable it before the send
    // XXX: what if the send blocks? Let's hope that someone else enabled it, and the ISR doesn't race and drain the queue before this...?
    UART_EnableIntr(UART0, UART_TXFIFO_EMPTY_INT_ENA);
    return 0;
  } else {
    return -1; // queue full?
  }
}

int uart_putc(int c)
{
  uint8_t tx_byte = c;
  int ret = 1;

  taskENTER_CRITICAL();
  {
    if (!uxQueueMessagesWaiting(uart.tx_queue)) {
      // fastpath
      if (UART_TryWrite(UART0, tx_byte)) {
        ret = 0; // ok
      }
    }
  }
  taskEXIT_CRITICAL();

  if (ret) {
    // slow path
    struct uart_event tx_event;

    tx_event.len = 1;
    tx_event.buf[0] = tx_byte;

    if (uart_tx(&tx_event) < 0) {
      ret = 1; // queue full
    } else {
      ret = 0; // ok
    }
  }

  return ret;
}

size_t uart_write(const void *buf, size_t len)
{
  const uint8_t *ptr = buf;
  int write = 0;

  taskENTER_CRITICAL();
  {
    if (!uxQueueMessagesWaiting(uart.tx_queue)) {
      // fastpath
      write = UART_Write(UART0, buf, len);
      ptr += write;
      len -= write;
    }
  }
  taskEXIT_CRITICAL();

  while (len > 0) {
    // slow path
    struct uart_event tx_event;
    size_t size = sizeof(tx_event.buf);

    if (len > size) {
      tx_event.len = size;
      memcpy(tx_event.buf, ptr, size);
      ptr += size;
      len -= size;
    } else {
      tx_event.len = len;
      memcpy(tx_event.buf, ptr, len);
      ptr += len;
      len = 0;
    }

    if (uart_tx(&tx_event) < 0) {
      break;
    } else {
      write += tx_event.len;
    }
  }

  return write;
}

static void uart_intr_handler(void *arg)
{
  uint32 intr_status = UART_GetIntrStatus(UART0);
  portBASE_TYPE tx_task_woken, rx_task_woken;

  if (intr_status & (UART_TXFIFO_EMPTY_INT_ST)) {
    uart_intr_tx(&tx_task_woken);
  }

  if (intr_status & (UART_RXFIFO_FULL_INT_ST | UART_RXFIFO_TOUT_INT_ST)) {
    uart_intr_rx(&rx_task_woken);
  }

  if (intr_status & (UART_RXFIFO_OVF_INT_ST)) {
    uart_intr_rx_overflow();
  }

  UART_ClearIntrStatus(UART0, UART_TXFIFO_EMPTY_INT_CLR | UART_RXFIFO_OVF_INT_CLR | UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);

  portEND_SWITCHING_ISR(tx_task_woken || rx_task_woken);
}

int init_uart(struct user_config *config)
{
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
    .enable_mask        = UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_OVF_INT_ENA | UART_RXFIFO_TOUT_INT_ENA, // XXX: zero INTR_ENA mask bugs out
    .rx_full_thresh     = UART_EVENT_SIZE, // more than bytes in rx fifo
    .rx_timeout_thresh  = 2, // time = 8-bits/baud-rate
    .tx_empty_thresh    = UART_TX_SIZE / 2, // fewer than bytes in tx fifo
  };

  if ((uart.tx_queue = xQueueCreate(UART_TX_QUEUE_SIZE, sizeof(struct uart_event))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  UART_WaitTxFifoEmpty(UART0);
  UART_Setup(UART0, &uart_config);
  UART_SetupIntr(UART0, &intr_config);

  UART_RegisterIntrHandler(&uart_intr_handler, NULL);

  ETS_UART_INTR_ENABLE();

  LOG_INFO("setup baud=%u", uart_config.baud_rate);

  return 0;
}

int uart_start_recv(xQueueHandle rx_queue)
{
  // direct uart_rx() ISR
  uart.rx_queue = rx_queue;

  return 0;
}
