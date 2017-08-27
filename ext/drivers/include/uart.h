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

#ifndef __UART_H__
#define __UART_H__

#include "uart_config.h"

#include <c_types.h>
#include <esp8266/esp8266.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ETS_UART_INTR_ENABLE()  _xt_isr_unmask(1 << ETS_UART_INUM)
#define ETS_UART_INTR_DISABLE() _xt_isr_mask(1 << ETS_UART_INUM)
#define UART_INTR_MASK          0x1ff
#define UART_LINE_INV_MASK      (0x3f<<19)
#define UART_TX_FIFO 128
#define UART_RX_FIFO 128

typedef enum {
    UART0 = 0x0,
    UART1 = 0x1,
} UART_Port;

typedef void (*UART_IntrHandlerFunc)(void *arg);

typedef struct {
    uint32 enable_mask;
    uint8  rx_timeout_thresh;
    uint8  rx_full_thresh;
    uint8  tx_empty_thresh;
} UART_IntrConfig;

//=======================================

/** \defgroup Driver_APIs Driver APIs
  * @brief Driver APIs
  */

/** @addtogroup Driver_APIs
  * @{
  */

/** \defgroup UART_Driver_APIs UART Driver APIs
  * @brief UART driver APIs
  */

/** @addtogroup UART_Driver_APIs
  * @{
  */

/**
  * @brief   Wait uart tx fifo empty, do not use it if tx flow control enabled.
  *
  * @param   UART_Port uart_no:UART0 or UART1
  *
  * @return  null
  */
void UART_WaitTxFifoEmpty(UART_Port uart_no); //do not use if tx flow control enabled

/**
 * @return number of bytes writable
 */
size_t UART_GetWriteSize(UART_Port uart_no);

/** Block waiting for one byte write to TX FIFO
  *
  * @param   uart_no : UART0 or UART1
  * @param   tx_char: char to write
  */
void UART_WaitWrite(UART_Port uart_no, uint8 tx_char);

/** Try writing one byte to TX FIFO
  *
  * @param   uart_no : UART0 or UART1
  * @param   tx_char: char to write
  * @return  0 if TX FIFO full, >0 if written
  */
int UART_TryWrite(UART_Port uart_no, uint8 tx_char);

/**
  * @param   port_no UART0 or UART1
  * @param   buf copy bytes from buffer
  * @param   len number of bytes in buffer
  * @return  number of bytes written
  */
size_t UART_Write(UART_Port uart_no, const void *buf, size_t len);

/**
  * @param   uart_no UART0 or UART1
  * @param   buf copy bytes to buffer
  * @param   size size of buffer
  * @return  number of bytes read
  */
size_t UART_Read(UART_Port uart_no, void *buf, size_t size);

/**
  * @brief   Clear uart tx fifo and rx fifo.
  *
  * @param   UART_Port uart_no : UART0 or UART1
  *
  * @return  null
  */
void UART_ResetFifo(UART_Port uart_no);

/**
  * @brief  Clear uart interrupt flags.
  *
  * @param   UART_Port uart_no : UART0 or UART1
  * @param   uint32 clr_mask : To clear the interrupt bits
  *
  * @return  null
  */
void UART_ClearIntrStatus(UART_Port uart_no, uint32 clr_mask);

/**
  * @brief   Enable uart interrupts .
  *
  * @param   UART_Port uart_no : UART0 or UART1
  * @param   uint32 ena_mask : interrupt mask
  *
  * @return  null
  */
void UART_SetIntrEna(UART_Port uart_no, uint32 ena_mask);

/**
  * @brief   Partially enable uart interrupts.
  *
  * @param   UART_Port uart_no : UART0 or UART1
  * @param   uint32 set_mask : set mask bits
  *
  * @return  null
  */
void UART_EnableIntr(UART_Port uart_no, uint32 set_mask);

/**
  * @brief   Partially disable uart interrupts.
  *
  * @param   UART_Port uart_no : UART0 or UART1
  * @param   uint32 set_mask : clear mask bits
  *
  * @return  null
  */
void UART_DisableIntr(UART_Port uart_no, uint32 clear_mask);

/**
  * @brief   Register an application-specific interrupt handler for Uarts interrupts.
  *
  * @param   UART_IntrHandlerFunc func : interrupt handler for Uart interrupts.
  * @param   void *arg : interrupt handler's arg.
  *
  * @return  null
  */
void UART_RegisterIntrHandler(UART_IntrHandlerFunc func, void *arg);

uint32 UART_GetIntrStatus(UART_Port port_no);

/**
  * @brief   Config Common parameters of serial ports.
  *
  * @param   UART_Port uart_no : UART0 or UART1
  * @param   UART_Config *pUARTConfig : parameters structure
  *
  * @return  null
  */
void UART_Setup(UART_Port uart_no, const UART_Config *pUARTConfig);

/**
  * @brief   Config types of uarts.
  *
  * @param   UART_Port uart_no : UART0 or UART1
  * @param   UART_IntrConfig *pUARTIntrConf : parameters structure
  *
  * @return  null
  */
void UART_SetupIntr(UART_Port uart_no, const UART_IntrConfig *pUARTIntrConf);

/**
  * @brief   Config the length of the uart communication data bits.
  *
  * @param   UART_Port uart_no : UART0 or UART1
  * @param   UART_WordLength len : the length of the uart communication data bits
  *
  * @return  null
  */
void UART_SetWordLength(UART_Port uart_no, UART_WordLength len);

/**
  * @brief   Config the length of the uart communication stop bits.
  *
  * @param   UART_Port uart_no : UART0 or UART1
  * @param   UART_StopBits bit_num : the length uart communication stop bits
  *
  * @return  null
  */
void UART_SetStopBits(UART_Port uart_no, UART_StopBits bit_num);

/**
  * @brief   Configure whether to open the parity.
  *
  * @param   UART_Port uart_no : UART0 or UART1
  * @param   UART_ParityMode Parity_mode : the enum of uart parity configuration
  *
  * @return  null
  */
void UART_SetParity(UART_Port uart_no, UART_ParityMode Parity_mode);

/**
 * @param uart_no: UART0/UART1
 * @param tx_break: drive line low after TX idle, normally high
 */
void UART_SetTxBreak(UART_Port uart_no, bool tx_break);

/**
  * @brief   Configure the Baud rate.
  *
  * @param   UART_Port uart_no : UART0 or UART1
  * @param   uint32 baud_rate : the Baud rate
  *
  * @return  null
  */
void UART_SetBaudRate(UART_Port uart_no, UART_BaudRate baud_rate);

/**
  * @brief   Configure Hardware flow control.
  *
  * @param   UART_Port uart_no : UART0 or UART1
  * @param   UART_HwFlowCtrl flow_ctrl : Hardware flow control mode
  * @param   uint8 rx_thresh : threshold of Hardware flow control
  *
  * @return  null
  */
void UART_SetFlowCtrl(UART_Port uart_no, UART_HwFlowCtrl flow_ctrl, uint8 rx_thresh);

/**
  * @brief   Configure trigging signal of uarts.
  *
  * @param   UART_Port uart_no : UART0 or UART1
  * @param   UART_LineLevelInverse inverse_mask : Choose need to flip the IO
  *
  * @return  null
  */
void UART_SetLineInverse(UART_Port uart_no, UART_LineLevelInverse inverse_mask) ;

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif
