#include "uart.h"
#include "isr.h"

// init() called
static bool init = false;

static struct uart_intr_handler {
  uart_intr_func_t func;
  void *arg;
} uart_intr_handler[UART_PORT_MAX] = {
  [UART_0]   = {},
  [UART_1]   = {},
};

static inline int uart_isr_call(enum uart_port intr)
{
  struct uart_intr_handler *handler = &uart_intr_handler[intr & UART_PORT_MASK];

  if (handler->func) {
    handler->func(handler->arg);

    return 1;
  } else {
    return 0;
  }
}

void IRAM_ATTR uart_isr(void *ctx)
{
  uint32_t int_status = READ_PERI_REG(REG_UART_INT_STATUS);

  if (int_status & UART_INT_STATUS_UART0) {
    if (!uart_isr_call(UART_0)) {
      // clear and disable
      uart0_intr_off();
    }
  }

  if (int_status & UART_INT_STATUS_UART1) {
    if (!uart_isr_call(UART_1)) {
      // clear and disable
      uart1_intr_off();
    }
  }
}

void uart_isr_init()
{
  if (init) {
    return;
  }

  uart_isr_attach(uart_isr, NULL);
  uart_isr_unmask();

  init = true;
}

void uart_isr_setup(enum uart_port port, uart_intr_func_t func, void *arg)
{
  uart_intr_handler[port & UART_PORT_MASK] = (struct uart_intr_handler) { func, arg };
}

void uart_isr_teardown(enum uart_port port)
{
  uart_intr_handler[port & UART_PORT_MASK] = (struct uart_intr_handler) { NULL, NULL };
}
