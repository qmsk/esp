#include "uart.h"

#include <drivers/uart.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "user_config.h"
#include "logging.h"

struct uart {
  UART_Port port;
  xQueueHandle tx_queue;
  xQueueHandle rx_queue;
  bool rx_overflow;
};

struct uart uart0 = {
  .port = UART0,
};
struct uart uart1 = {
  .port = UART1,
};

// drain TX queue by writing to UART FIFO
// Leaves the TXFIFO_EMPTY interrupt enabled if the TX buffer still has data
static void uart_intr_tx(struct uart *uart, portBASE_TYPE *task_wokenp) // ISR UART_TXFIFO_EMPTY_INT_ST
{
  struct uart_io tx;

  while (true) {
    if (UART_GetWriteSize(uart->port) < sizeof(tx.buf)) {
      // not enough room in the UART TX fifo to write out a complete event
      break;
    }

    if (!xQueueReceiveFromISR(uart->tx_queue, &tx, task_wokenp)) {
      // queue empty
      UART_DisableIntr(uart->port, UART_TXFIFO_EMPTY_INT_ENA);

      break;
    }

    size_t write = 0;

    do {
      // this should happen in a single write assuming the UART_GetWriteSize() check
      write += UART_Write(uart->port, tx.buf + write, tx.len - write);
    } while (write < tx.len);
  }
}

static void uart_intr_rx(struct uart *uart, portBASE_TYPE *task_wokenp) // ISR UART_RXFIFO_FULL_INT_ST | UART_RXFIFO_TOUT_INT_ST
{
  struct uart_io rx = {.flags = 0};

  if (uart->rx_overflow) {
    rx.flags |= UART_RX_OVERFLOW;
  }

  // must read to clear the interrupt, even if queue overflow
  rx.len = UART_Read(uart->port, rx.buf, sizeof(rx.buf));

  if (!uart->rx_queue) {
    uart->rx_overflow = true;
  } else if (xQueueSendFromISR(uart->rx_queue, &rx, task_wokenp) <= 0) {
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
int uart_tx(struct uart *uart, const struct uart_io *tx)
{
  if (xQueueSend(uart->tx_queue, tx, portMAX_DELAY)) {
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
    struct uart_io tx;

    tx.len = 1;
    tx.buf[0] = tx_byte;

    if (uart_tx(uart, &tx) < 0) {
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
    struct uart_io tx;
    size_t size = sizeof(tx.buf);

    if (len > size) {
      tx.len = size;
      memcpy(tx.buf, ptr, size);
      ptr += size;
      len -= size;
    } else {
      tx.len = len;
      memcpy(tx.buf, ptr, len);
      ptr += len;
      len = 0;
    }

    if (uart_tx(uart, &tx) < 0) {
      break;
    } else {
      write += tx.len;
    }
  }

  return write;
}

int uart_read(struct uart *uart, void *buf, size_t size)
{
  struct uart_io rx;

  if (!uart->rx_queue) {
    LOG_WARN("rx disabled");
    return -1;
  }

  if (size < sizeof(rx.buf)) {
    LOG_ERROR("buf size %u < %u", size, sizeof(rx.buf));
    return -1;
  }

  if (!xQueueReceive(uart->rx_queue, &rx, portMAX_DELAY)) {
    LOG_WARN("xQueueReceive"); // XXX
    return 0; // TODO: abort/timeout?
  }

  if (rx.flags & UART_RX_OVERFLOW) {
    // discard input before overflow
    LOG_ERROR("rx overflow");
    return -1;
  }

  memcpy(buf, rx.buf, rx.len);

  return rx.len;
}

static void uart_intr_handler(void *arg)
{
  uint32 intr0_status = UART_GetIntrStatus(UART0);
  uint32 intr1_status = UART_GetIntrStatus(UART1);
  portBASE_TYPE tx0_task_woken, rx0_task_woken, tx1_task_woken;

  if (intr0_status & (UART_TXFIFO_EMPTY_INT_ST)) {
    uart_intr_tx(&uart0, &tx0_task_woken);
  }

  if (intr0_status & (UART_RXFIFO_FULL_INT_ST | UART_RXFIFO_TOUT_INT_ST)) {
    uart_intr_rx(&uart0, &rx0_task_woken);
  }

  if (intr0_status & (UART_RXFIFO_OVF_INT_ST)) {
    uart_intr_rx_overflow(&uart0);
  }

  if (intr1_status & (UART_TXFIFO_EMPTY_INT_ST)) {
    uart_intr_tx(&uart1, &tx1_task_woken);
  }

  UART_ClearIntrStatus(UART0, UART_TXFIFO_EMPTY_INT_CLR | UART_RXFIFO_OVF_INT_CLR | UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);
  UART_ClearIntrStatus(UART1, UART_TXFIFO_EMPTY_INT_CLR);

  portEND_SWITCHING_ISR(tx0_task_woken || rx0_task_woken || tx1_task_woken);
}

static int uart_init(struct uart *uart, size_t tx_queue, size_t rx_queue)
{
  if ((uart->tx_queue = xQueueCreate(tx_queue, sizeof(struct uart_io))) == NULL) {
    LOG_ERROR("xQueueCreate TX %u*%u", tx_queue, sizeof(struct uart_io));
    return -1;
  }

  if (!rx_queue) {
    uart->rx_queue = NULL; // disabled
  } else if ((uart->rx_queue = xQueueCreate(rx_queue, sizeof(struct uart_io))) == NULL) {
    LOG_ERROR("xQueueCreate RX %u*%u", rx_queue, sizeof(struct uart_io));
    return -1;
  }

  return 0;
}

int setup_uart(struct uart *uart, UART_Config *uart_config)
{
  UART_IntrConfig intr_config = {
    .enable_mask        = UART_RXFIFO_FULL_INT_ENA, // XXX: zero INTR_ENA mask bugs out
    .rx_full_thresh     = UART_IO_SIZE, // more than bytes in rx fifo
    .rx_timeout_thresh  = 2, // time = 8-bits/baud-rate
    .tx_empty_thresh    = UART_TX_FIFO / 2, // fewer than bytes in tx fifo
  };

  if (uart->rx_queue) {
    intr_config.enable_mask |= UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_OVF_INT_ENA | UART_RXFIFO_TOUT_INT_ENA;
  }

  UART_WaitTxFifoEmpty(uart->port);
  UART_Setup(uart->port, uart_config);
  UART_SetupIntr(uart->port, &intr_config);

  return 0;
}

void disable_uart(struct uart *uart)
{
  UART_ClearIntrStatus(uart->port, UART_INTR_MASK);
  UART_SetIntrEna(uart->port, 0);
}

int init_uart(struct user_config *config)
{
  int err;

  UART_Config uart_config = {
    .baud_rate  = USER_CONFIG_UART_BAUD_RATE,
    .data_bits  = UART_WordLength_8b,
    .parity     = UART_Parity_None,
    .stop_bits  = UART_StopBits_1,
  };

  if ((err = uart_init(&uart0, UART_TX_QUEUE_SIZE, UART_RX_QUEUE_SIZE))) {
    LOG_ERROR("uart_init uart0");
    return err;
  }

  if ((err = uart_init(&uart1, UART_TX_QUEUE_SIZE, 0))) {
    LOG_ERROR("uart_init uart1");
    return err;
  }

  if ((err = setup_uart(&uart0, &uart_config))) {
    LOG_ERROR ("setup_uart uart0");
    return err;
  }

  disable_uart(&uart1);

  UART_RegisterIntrHandler(&uart_intr_handler, NULL);

  ETS_UART_INTR_ENABLE();

  LOG_INFO("setup baud=%u", uart_config.baud_rate);

  return 0;
}
