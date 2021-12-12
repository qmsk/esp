#pragma once

#include <esp8266/spi_struct.h>
#include <esp8266/spi_register.h>
#include <esp8266/i2s_struct.h>
#include <esp8266/i2s_register.h>

/* Undocumented */
#define REG_SPI_INT_STATUS 0x3ff00020
#define SPI_INT_STATUS_SPI0 BIT4
#define SPI_INT_STATUS_SPI1 BIT7
#define SPI_INT_STATUS_I2S BIT9

/*
 * Note that the SPI0 interrupt is likely to fire immediately once the ISR is attached/unmasked, and MUST be disabled or the system will soft-lockup.
 */

#define SPI_SLAVE_INT_ENA_BITS (SPI_TRANS_DONE_EN | SPI_SLV_WR_STA_DONE_EN | SPI_SLV_RD_STA_DONE_EN | SPI_SLV_WR_BUF_DONE_EN | SPI_SLV_RD_BUF_DONE_EN)
#define SPI_SLAVE_INT_RAW_BITS (SPI_TRANS_DONE | SPI_SLV_WR_STA_DONE | SPI_SLV_RD_STA_DONE | SPI_SLV_WR_BUF_DONE | SPI_SLV_RD_BUF_DONE)

static inline void spi0_intr_off()
{
  // clear and disable
  SPI0.slave.val &= ~(SPI_SLAVE_INT_ENA_BITS | SPI_SLAVE_INT_RAW_BITS);
}

static inline void spi1_intr_off()
{
  // clear and disable
  SPI1.slave.val &= ~(SPI_SLAVE_INT_ENA_BITS | SPI_SLAVE_INT_RAW_BITS);
}

static inline void i2s_intr_off()
{
  // clear and disable
  I2S0.int_ena.val &= ~(I2S_I2S_TX_REMPTY_INT_ENA | I2S_I2S_TX_WFULL_INT_ENA | I2S_I2S_RX_REMPTY_INT_ENA | I2S_I2S_RX_WFULL_INT_ENA | I2S_I2S_TX_PUT_DATA_INT_ENA | I2S_I2S_RX_TAKE_DATA_INT_ENA);
  I2S0.int_clr.val |= (I2S_I2S_TX_REMPTY_INT_CLR | I2S_I2S_TX_WFULL_INT_CLR | I2S_I2S_RX_REMPTY_INT_CLR | I2S_I2S_RX_WFULL_INT_CLR | I2S_I2S_PUT_DATA_INT_CLR | I2S_I2S_TAKE_DATA_INT_CLR);
}
