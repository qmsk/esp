#include <spi_master.h>
#include "spi_master.h"
#include "spi_dev.h"
#include <logging.h>

#include <esp8266/pin_mux_register.h>

#define SPI_CLK_EQU_SYS_CLK (SPI1_CLK_EQU_SYS_CLK)

#define SPI_PERIPHS_IO_MUX_CLK (PERIPHS_IO_MUX_MTMS_U)
#define SPI_FUNC_CLK (FUNC_HSPI_CLK)

#define SPI_PERIPHS_IO_MUX_MOSI (PERIPHS_IO_MUX_MTCK_U)
#define SPI_FUNC_MOSI (FUNC_HSPID_MOSI)

#define SPI_PERIPHS_IO_MUX_MISO (PERIPHS_IO_MUX_MTDI_U)
#define SPI_FUNC_MISO (FUNC_HSPIQ_MISO)

#define SPI_PERIPHS_IO_MUX_CS (PERIPHS_IO_MUX_MTDO_U)
#define SPI_FUNC_CS (FUNC_HSPI_CS0)

int spi_master_mode(struct spi_master *spi_master, enum spi_mode mode)
{
  bool cpol = (mode & SPI_MODE_CPOL_HIGH) ? true : false;
  bool cpha = (mode & SPI_MODE_CPHA_HIGH) ? true : false;

  spi_master->mode = (mode & SPI_MODE_FLAGS);

  SPI_DEV.pin.slave_mode = 0;
  SPI_DEV.pin.ck_idle_edge = cpol ? 1 : 0;

  SPI_DEV.ctrl.fread_dual = 0;
  SPI_DEV.ctrl.fread_quad = 0;
  SPI_DEV.ctrl.fread_dio = 0;
  SPI_DEV.ctrl.fread_qio = 0;
  SPI_DEV.ctrl.fastrd_mode = 0;
  SPI_DEV.ctrl.rd_bit_order = (mode & SPI_MODE_LSB_FIRST) ? 1 : 0;
  SPI_DEV.ctrl.wr_bit_order = (mode & SPI_MODE_LSB_FIRST) ? 1 : 0;

  // TODO: ctrl2.{miso,mosi,cs}_delay_{mode,num}?
  SPI_DEV.ctrl2.mosi_delay_num = 0;
  SPI_DEV.ctrl2.miso_delay_num = 1;

  SPI_DEV.user.flash_mode = 0;
  SPI_DEV.user.cs_hold = 1;
  SPI_DEV.user.cs_setup = 1;
  SPI_DEV.user.ck_i_edge = 1;
  SPI_DEV.user.ck_out_edge = (cpol == cpha) ? 0 : 1;
  SPI_DEV.user.rd_byte_order = (mode & SPI_MODE_BE_ORDER) ? 1 : 0;
  SPI_DEV.user.wr_byte_order = (mode & SPI_MODE_BE_ORDER) ? 1 : 0;
  SPI_DEV.user.fwrite_dual = 0;
  SPI_DEV.user.fwrite_quad = 0;
  SPI_DEV.user.fwrite_dio = 0;
  SPI_DEV.user.fwrite_qio = 0;
  SPI_DEV.user.usr_mosi_highpart = 0;
  SPI_DEV.user.usr_miso_highpart = 0;

  SPI_DEV.slave.slave_mode = 0;
  SPI_DEV.slave.sync_reset = 1;

  return 0;
}

int spi_master_clock(struct spi_master *spi_master, enum spi_clock clock)
{
  int div, cnt;

  spi_master->clock = clock;

  if (clock == 1) {
    SET_PERI_REG_MASK(PERIPHS_IO_MUX_CONF_U, SPI_CLK_EQU_SYS_CLK);
  } else {
    CLEAR_PERI_REG_MASK(PERIPHS_IO_MUX_CONF_U, SPI_CLK_EQU_SYS_CLK);
  }

  if (clock > 40) {
    div = (clock / 40);
    cnt = (clock / div);
  } else {
    div = 1;
    cnt = clock;
  }

  SPI_DEV.clock.clk_equ_sysclk = (clock == 1) ? 1 : 0;
  SPI_DEV.clock.clkdiv_pre = div - 1;
  SPI_DEV.clock.clkcnt_n = cnt - 1;
  SPI_DEV.clock.clkcnt_h = cnt / 2 - 1;
  SPI_DEV.clock.clkcnt_l = cnt - 1;

  return 0;
}

int spi_master_pins(struct spi_master *spi_master, enum spi_pins pins)
{
  if (pins & SPI_PINS_CLK) {
    PIN_PULLUP_EN(SPI_PERIPHS_IO_MUX_CLK);
    PIN_FUNC_SELECT(SPI_PERIPHS_IO_MUX_CLK, SPI_FUNC_CLK);
  }

  if (pins & SPI_PINS_MOSI) {
    PIN_PULLUP_EN(SPI_PERIPHS_IO_MUX_MOSI);
    PIN_FUNC_SELECT(SPI_PERIPHS_IO_MUX_MOSI, SPI_FUNC_MOSI);
  }

  if (pins & SPI_PINS_MISO) {
    PIN_PULLUP_EN(SPI_PERIPHS_IO_MUX_MISO);
    PIN_FUNC_SELECT(SPI_PERIPHS_IO_MUX_MISO, SPI_FUNC_MISO);
  }

  if (pins & SPI_PINS_CS) {
    PIN_PULLUP_EN(SPI_PERIPHS_IO_MUX_CS);
    PIN_FUNC_SELECT(SPI_PERIPHS_IO_MUX_CS, SPI_FUNC_CS);
  }

  return 0;
}

int spi_master_init(struct spi_master *spi_master, const struct spi_options options)
{
  int err;

  if (!(spi_master->mutex = xSemaphoreCreateMutex())) {
    LOG_ERROR("xSemaphoreCreateMutex");
  }

  if ((err = spi_master_mode(spi_master, options.mode))) {
    LOG_ERROR("spi_master_mode");
    return err;
  }

  if ((err = spi_master_pins(spi_master, options.pins))) {
    LOG_ERROR("spi_master_pins");
    return err;
  }

  if ((err = spi_master_clock(spi_master, options.clock))) {
    LOG_ERROR("spi_master_clock");
    return err;
  }

  return 0;
}
