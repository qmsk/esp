#pragma once

#include <esp8266/uart_struct.h>
#include <esp8266/uart_register.h>

#include "freertos/FreeRTOS.h"
#include "rom/ets_sys.h"

/* Undocumented */
#define REG_UART_INT_STATUS 0x3ff00020
#define UART_INT_STATUS_UART0 BIT0
#define UART_INT_STATUS_UART1 BIT2

/*
 * Note that the SPI0 interrupt is likely to fire immediately once the ISR is attached/unmasked, and MUST be disabled or the system will soft-lockup.
 */
#define UART_INT_ENA_BITS (UART_RXFIFO_TOUT_INT_ENA | UART_CTS_CHG_INT_ENA | UART_DSR_CHG_INT_ENA | UART_RXFIFO_OVF_INT_ENA | UART_FRM_ERR_INT_ENA | UART_PARITY_ERR_INT_ENA | UART_TXFIFO_EMPTY_INT_ENA | UART_RXFIFO_FULL_INT_ENA)
#define UART_INT_CLR_BITS (UART_RXFIFO_TOUT_INT_CLR | UART_BRK_DET_INT_CLR | UART_CTS_CHG_INT_CLR | UART_DSR_CHG_INT_CLR | UART_RXFIFO_OVF_INT_CLR | UART_FRM_ERR_INT_CLR | UART_PARITY_ERR_INT_CLR | UART_TXFIFO_EMPTY_INT_CLR | UART_RXFIFO_FULL_INT_CLR)

static inline void uart_isr_unmask()
{
  _xt_isr_unmask(1 << ETS_UART_INUM);
}

static inline void uart_isr_mask()
{
  _xt_isr_mask(1 << ETS_UART_INUM);
}

static inline void uart_isr_attach(_xt_isr func, void *arg)
{
  _xt_isr_attach(ETS_UART_INUM, func, arg);
}

void IRAM_ATTR uart_isr(void *arg);


static inline void uart0_intr_off()
{
  // clear and disable
  uart0.int_clr.val = UART_INT_CLR_BITS;
  uart0.int_ena.val = 0;
}
static inline void uart1_intr_off()
{
  // clear and disable
  uart1.int_clr.val = UART_INT_CLR_BITS;
  uart1.int_ena.val = 0;
}
