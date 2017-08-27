/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __UART_CONFIG_H__
#define __UART_CONFIG_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UART_WordLength_5b = 0x0,
    UART_WordLength_6b = 0x1,
    UART_WordLength_7b = 0x2,
    UART_WordLength_8b = 0x3
} UART_WordLength;

typedef enum {
    UART_StopBits_1   = 0x1,
    UART_StopBits_1_5 = 0x2,
    UART_StopBits_2   = 0x3,
} UART_StopBits;

typedef enum {
  UART_Parity_None = 0x0,
  UART_Parity_Even = 0x2,
  UART_Parity_Odd  = 0x3,
} UART_ParityMode;

typedef enum {
    BAUD_RATE_300     = 300,
    BAUD_RATE_600     = 600,
    BAUD_RATE_1200    = 1200,
    BAUD_RATE_2400    = 2400,
    BAUD_RATE_4800    = 4800,
    BAUD_RATE_9600    = 9600,
    BAUD_RATE_19200   = 19200,
    BAUD_RATE_38400   = 38400,
    BAUD_RATE_57600   = 57600,
    BAUD_RATE_74880   = 74880,
    BAUD_RATE_115200  = 115200,
    BAUD_RATE_230400  = 230400,
    BAUD_RATE_460800  = 460800,
    BAUD_RATE_921600  = 921600,
    BAUD_RATE_1843200 = 1843200,
    BAUD_RATE_3686400 = 3686400,
} UART_BaudRate; //you can add any rate you need in this range

typedef enum {
    USART_HardwareFlowControl_None    = 0x0,
    USART_HardwareFlowControl_RTS     = 0x1,
    USART_HardwareFlowControl_CTS     = 0x2,
    USART_HardwareFlowControl_CTS_RTS = 0x3
} UART_HwFlowCtrl;

typedef enum {
    UART_None_Inverse = 0x0,
    UART_Rxd_Inverse  = 1 << 19, // UART_RXD_INV
    UART_CTS_Inverse  = 1 << 20, // UART_CTS_INV
    UART_Txd_Inverse  = 1 << 22, // UART_TXD_INV
    UART_RTS_Inverse  = 1 << 23, // UART_RTS_INV
} UART_LineLevelInverse;

typedef struct {
    UART_BaudRate   baud_rate;
    UART_WordLength data_bits;
    UART_ParityMode parity;    // chip size in byte
    UART_StopBits   stop_bits;
    UART_HwFlowCtrl flow_ctrl;
    uint8_t         flow_rx_thresh;
    UART_LineLevelInverse inverse_mask;
} UART_Config;

#ifdef __cplusplus
}
#endif

#endif
