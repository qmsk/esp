#define DEBUG

#include "spi.h"
#include "spi_register.h"

#include <c_types.h>
#include <esp8266/eagle_soc.h>
#include <esp8266/pin_mux_register.h>


#define WRITE_PERI_REG_MASK(reg, mask, bit) WRITE_PERI_REG((reg), ((READ_PERI_REG(reg) & (~(mask))) | ((bit) ? (mask) : 0)))
#define WRITE_PERI_REG_BITS(reg, mask, bits) WRITE_PERI_REG((reg), ((READ_PERI_REG(reg) & (~(mask)))) | ((bits) & (mask)))

#ifdef DEBUG
  #include <esp_libc.h>

  #define SPI_DEBUG_REG(reg) os_printf("\t%-16s @ 0x%08x = %08x\n", #reg, reg, READ_PERI_REG(reg))

  void SPI_Debug(enum SPI spi, const char *prefix)
  {
    os_printf("SPI%d @ %s:\n", spi, prefix);

    SPI_DEBUG_REG(SPI_CMD(spi));
    SPI_DEBUG_REG(SPI_ADDR(spi));
    SPI_DEBUG_REG(SPI_CTRL(spi));
    SPI_DEBUG_REG(SPI_CTRL1(spi));
    SPI_DEBUG_REG(SPI_RD_STATUS(spi));
    SPI_DEBUG_REG(SPI_CTRL2(spi));
    SPI_DEBUG_REG(SPI_CLOCK(spi));
    SPI_DEBUG_REG(SPI_USER(spi));
    SPI_DEBUG_REG(SPI_USER1(spi));
    SPI_DEBUG_REG(SPI_USER2(spi));
    SPI_DEBUG_REG(SPI_WR_STATUS(spi));
    SPI_DEBUG_REG(SPI_PIN(spi));
    SPI_DEBUG_REG(SPI_SLAVE(spi));
    SPI_DEBUG_REG(SPI_SLAVE1(spi));
    SPI_DEBUG_REG(SPI_SLAVE2(spi));
    SPI_DEBUG_REG(SPI_SLAVE3(spi));
    SPI_DEBUG_REG(SPI_W0(spi));
    SPI_DEBUG_REG(SPI_EXT2(spi));
    SPI_DEBUG_REG(SPI_EXT3(spi));
  }

  #define SPI_DEBUG(spi, prefix) SPI_Debug(spi, prefix)
#else
  #define SPI_DEBUG(spi, prefix)
#endif

void SPI_SelectPinFunc(enum SPI spi)
{
  switch (spi) {
    case SPI_1:
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_HSPIQ_MISO); // TODO: optional
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_HSPID_MOSI);
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_HSPI_CLK);
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_HSPI_CS0); // TODO: optional
      break;
  }
}

void SPI_SetClock(enum SPI spi, enum SPI_Clock clkdiv)
{
  uint32_t io_mux_conf_mask = 0;

  switch(spi) {
    case SPI_1:
      io_mux_conf_mask = SPI1_CLK_EQU_SYS_CLK;
  }

  WRITE_PERI_REG_MASK(PERIPHS_IO_MUX_CONF_U, io_mux_conf_mask, (clkdiv == 0));

  if (clkdiv > 0) {
    uint16_t clkdiv_pre = (clkdiv > 40) ? (clkdiv / 40) : 1; // 13-bit
    uint16_t clkcnt_n = (clkdiv / clkdiv_pre); // 6-bit

    WRITE_PERI_REG(SPI_CLOCK(spi),
          ((clkdiv_pre - 1) & SPI_CLKDIV_PRE) << SPI_CLKDIV_PRE_S
      |   ((clkcnt_n - 1) & SPI_CLKCNT_N) << SPI_CLKCNT_N_S
      |   ((clkcnt_n / 2 - 1) & SPI_CLKCNT_H) << SPI_CLKCNT_H_S   // XXX: +1?
      |   ((clkcnt_n - 1) & SPI_CLKCNT_L) << SPI_CLKCNT_L_S
    );
  } else {
    WRITE_PERI_REG(SPI_CLOCK(spi), SPI_CLK_EQU_SYSCLK);
  }
}

void SPI_SetupMaster(enum SPI spi, struct SPI_MasterConfig config)
{
  bool mode_cpol = (config.mode & SPI_MODE_CPOL);
  bool mode_cpha = (config.mode & SPI_MODE_CPHA);
  SPI_DEBUG(spi, "setup-pre");

  WRITE_PERI_REG_MASK(SPI_PIN(spi), SPI_IDLE_EDGE, mode_cpol);
  CLEAR_PERI_REG_MASK(SPI_CTRL(spi),
      SPI_WR_BIT_ORDER  // 0 = MSB first
    | SPI_RD_BIT_ORDER  // 0 = MSB first
    | SPI_QIO_MODE | SPI_DIO_MODE // 0 = single line
    | SPI_DOUT_MODE | SPI_QOUT_MODE // 0 = single line
    // TODO: SPI_FASTRD_MODE?
  );
  CLEAR_PERI_REG_MASK(SPI_USER(spi),
      SPI_FLASH_MODE
      // TODO: SPI_USR_*_HIGHPART?
  );
  SET_PERI_REG_MASK(SPI_USER(spi),
      SPI_CS_SETUP | SPI_CS_HOLD // TODO: ???
    | SPI_WR_BYTE_ORDER // 1 = XXX: little-endian?
    | SPI_RD_BYTE_ORDER // 1 = XXX: little-endian?
  );
  WRITE_PERI_REG_BITS(SPI_USER(spi), SPI_CK_OUT_EDGE,
      ((mode_cpha != mode_cpol) ? SPI_CK_OUT_EDGE : 0)
  );
  CLEAR_PERI_REG_MASK(SPI_SLAVE(spi),
      SPI_SLAVE_MODE // 0 = master
  );

  // TODO: SPI_CTRL2 SPI_MISO_DELAY_NUM = 1?

  SPI_SetClock(spi, config.clock);
  SPI_SelectPinFunc(spi);

  SPI_DEBUG(spi, "setup-post");
}
