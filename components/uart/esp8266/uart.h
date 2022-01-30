#pragma once

#include <uart.h>

/* isr.c */
typedef void (* uart_intr_func_t)(void *arg);

void uart_isr_init();
void uart_isr_setup(uart_port_t, uart_intr_func_t func, void *arg);
void uart_isr_teardown(uart_port_t);
