/*
 *  Copyright (c) 2010 - 2011 Espressif System
 *
 */

#include <esp8266/spi_register.h>

#define SPI_CTRL1(i)                         (REG_SPI_BASE(i)  + 0xc)
#define  SPI_CS_HOLD_DELAY  0xf
#define  SPI_CS_HOLD_DELAY_S   28
#define  SPI_CS_HOLD_DELAY_RES  0xfff
#define  SPI_CS_HOLD_DELAY_RES_S   16

//SPI_PIN
#define SPI_IDLE_EDGE (BIT(29)) // @see http://bbs.espressif.com/viewtopic.php?f=49&t=1570
