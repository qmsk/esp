#include "uart.h"

#include <drivers/uart.h>
#include <freertos/task.h>

#include "user_config.h"
#include "logging.h"

struct uart {
  UART_Port port;
  xQueueHandle tx_queue;
  xQueueHandle rx_queue;
  bool rx_overflow;
};

struct uart uart0;

// drain TX queue by writing to UART FIFO
// Leaves the TXFIFO_EMPTY interrupt enabled if the TX buffer still has data
static void uart_intr_tx(struct uart *uart, portBASE_TYPE *task_wokenp) // ISR UART_TXFIFO_EMPTY_INT_ST
{
  struct uart_event tx_event;

  while (true) {
    if (UART_GetWriteSize(uart->port) < sizeof(tx_event.buf)) {
      // not enough room in the UART TX fifo to write out a complete event
      break;
    }

    if (!xQueueReceiveFromISR(uart->tx_queue, &tx_event, task_wokenp)) {
      // queue empty
      UART_DisableIntr(uart->port, UART_TXFIFO_EMPTY_INT_ENA);

      break;
    }

    uint8_t *buf = tx_event.buf;
    size_t len = tx_event.len;
    size_t write = 0;

    do {
      // this should happen in a single write assuming the UART_GetWriteSize() check
      write += UART_Write(uart->port, buf + write, len - write);
    } while (write < len);
  }
}

static void uart_intr_rx(struct uart *uart, portBASE_TYPE *task_wokenp) // ISR UART_RXFIFO_FULL_INT_ST | UART_RXFIFO_TOUT_INT_ST
{
  struct uart_event rx_event = {.flags = 0};

  if (uart->rx_overflow) {
    rx_event.flags |= UART_RX_OVERFLOW;
  }

  // must read to clear the interrupt, even if queue overflow
  rx_event.len = UART_Read(uart->port, rx_event.buf, sizeof(rx_event.buf));

  if (!uart->rx_queue) {
    uart->rx_overflow = true;
  } else if (xQueueSendFromISR(uart->rx_queue, &rx_event, task_wokenp) <= 0) {
    uart->rx_overflow = true;
  } else {
    uart->rx_overflow = false;
  }
}

static void uart_intr_rx_overflow(struct uart *uart) // ISR UART_RXFIFO_OVF_INT_ST
{
  // lost data after last read
  uart->rx_overflow = true;
}

// Push TX event to queue, enabling the TX interrupt
// Blocks if the queue is full
// @return <0 on timeout, 0 on success
int uart_tx(struct uart *uart, const struct uart_event *tx_event)
{
  if (xQueueSend(uart->tx_queue, tx_event, portMAX_DELAY)) {
    // once there is data on the queue, enable the TX interrupt to ensure that the ISR will fire and drain the queue
    // can't enable the TX interrupt before sending, or the ISR will race, see that the queue is empty and disable it, before we send...
    // if the queue is full, then the ISR will already be enabled by a previous send, and this will yield waiting for the ISR to drain the queue
    // assume that xQueueSend on a full queue is safe against a racing ISR that drains the queue, and thus does not yield to an empty queue with interrupts disabled by the ISR!
    // the ISR may have drained the queue by the time this returns, but re-enabling the interrupt should be harmless: the ISR will fire once and disable itself
    UART_EnableIntr(uart->port, UART_TXFIFO_EMPTY_INT_ENA);
    return 0;
  } else {
    return -1; // queue full?
  }
}

int uart_putc(struct uart *uart, int c)
{
  uint8_t tx_byte = c;
  int ret = 1;

  taskENTER_CRITICAL();
  {
    if (!uxQueueMessagesWaiting(uart->tx_queue)) {
      // fastpath
      if (UART_TryWrite(uart->port, tx_byte)) {
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

    if (uart_tx(uart, &tx_event) < 0) {
      ret = 1; // queue full
    } else {
      ret = 0; // ok
    }
  }

  return ret;
}

size_t uart_write(struct uart *uart, const void *buf, size_t len)
{
  const uint8_t *ptr = buf;
  int write = 0;

  taskENTER_CRITICAL();
  {
    if (!uxQueueMessagesWaiting(uart->tx_queue)) {
      // fastpath
      write = UART_Write(uart->port, buf, len);
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

    if (uart_tx(uart, &tx_event) < 0) {
      break;
    } else {
      write += tx_event.len;
    }
  }

  return write;
}

static void uart_intr_handler(void *arg)
{
  uint32 intr0_status = UART_GetIntrStatus(UART0);
  portBASE_TYPE tx_task_woken, rx_task_woken;

  if (intr0_status & (UART_TXFIFO_EMPTY_INT_ST)) {
    uart_intr_tx(&uart0, &tx_task_woken);
  }

  if (intr0_status & (UART_RXFIFO_FULL_INT_ST | UART_RXFIFO_TOUT_INT_ST)) {
    uart_intr_rx(&uart0, &rx_task_woken);
  }

  if (intr0_status & (UART_RXFIFO_OVF_INT_ST)) {
    uart_intr_rx_overflow(&uart0);
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

  uart0.port = UART0;

  if ((uart0.tx_queue = xQueueCreate(UART_TX_QUEUE_SIZE, sizeof(struct uart_event))) == NULL) {
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
  // direct uart_intr_rx()
  uart0.rx_queue = rx_queue;

  return 0;
}
