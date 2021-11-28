#include <uart0.h>
#include "uart0.h"

#include <logging.h>

#include <esp8266/uart_struct.h>
#include <esp8266/uart_register.h>
#include <esp8266/pin_mux_register.h>
#include <esp8266/eagle_soc.h>
#include <esp8266/rom_functions.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void uart0_setup(struct uart0_options options)
{
  LOG_DEBUG("clock_div=%d data_bits=%x parity_bits=%x stop_bits=%x inverted=%x",
    options.clock_div,
    options.data_bits,
    options.parity_bits,
    options.stop_bits,
    options.inverted
  );

  taskENTER_CRITICAL();

  uart0.clk_div.div_int = options.clock_div;

  uart0.conf0.parity = options.parity_bits & 0x1;
  uart0.conf0.parity_en = options.parity_bits ? 1 : 0;
  uart0.conf0.bit_num = options.data_bits;
  uart0.conf0.stop_bit_num = options.stop_bits;
  uart0.conf0.rxd_inv = options.rx_inverted ? 1 : 0;

  if (options.swap) {
    // swap UART0 CTS <-> RX and RTS <-> TX
    SET_PERI_REG_MASK(UART_SWAP_REG, 0x4);

    // GPIO13 UART0 CTS -> RX
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_UART0_CTS);

  } else {
    // swap UART0 CTS <-> RX and RTS <-> TX
    CLEAR_PERI_REG_MASK(UART_SWAP_REG, 0x4);

    // GPIO3 UART0 RX
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_U0RXD);
  }

  taskEXIT_CRITICAL();
}
