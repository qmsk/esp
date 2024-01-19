#include "stusb4500.h"
#include "stusb4500_i2c.h"
#include "stusb4500_nvm.h"

#include <esp_err.h>

#define DEBUG

#include <logging.h>

static int stusb4500_nvm_unlock(struct stusb4500 *stusb4500)
{
  struct stusb4500_ftp_cust_password password = { .password = stusb4500_ftp_cust_password_password };
  int err;

  if ((err = stusb4500_i2c_write(stusb4500, STUSB4500_FTP_CUST_PASSWORD, &password, sizeof(password)))) {
    LOG_ERROR("stusb4500_i2c_write STUSB4500_FTP_CUST_PASSWORD");
    return err;
  }

  return 0;
}

static int stusb4500_nvm_lock(struct stusb4500 *stusb4500)
{
  struct stusb4500_ftp_ctrl_0 ctrl0_reset = {};
  struct stusb4500_ftp_ctrl_0 ctrl0_off = { .rst_n = 1 };
  struct stusb4500_ftp_ctrl_1 ctrl1_clear = { };
  struct stusb4500_ftp_cust_password password_clear = { };
  int err;

  if ((err = stusb4500_i2c_write(stusb4500, STUSB4500_FTP_CTRL_0, &ctrl0_reset, sizeof(ctrl0_reset)))) {
    LOG_ERROR("stusb4500_i2c_write STUSB4500_FTP_CTRL_0");
    return err;
  }

  if ((err = stusb4500_i2c_write(stusb4500, STUSB4500_FTP_CTRL_0, &ctrl0_off, sizeof(ctrl0_off)))) {
    LOG_ERROR("stusb4500_i2c_write STUSB4500_FTP_CTRL_0");
    return err;
  }

  if ((err = stusb4500_i2c_write(stusb4500, STUSB4500_FTP_CTRL_1, &ctrl1_clear, sizeof(ctrl1_clear)))) {
    LOG_ERROR("stusb4500_i2c_write STUSB4500_FTP_CTRL_1");
    return err;
  }

  if ((err = stusb4500_i2c_write(stusb4500, STUSB4500_FTP_CUST_PASSWORD, &password_clear, sizeof(password_clear)))) {
    LOG_ERROR("stusb4500_i2c_write STUSB4500_FTP_CUST_PASSWORD");
    return err;
  }

  return 0;
}

static int stusb4500_nvm_reset(struct stusb4500 *stusb4500)
{
  struct stusb4500_ftp_ctrl_0 ctrl0_reset = {};
  struct stusb4500_ftp_ctrl_0 ctrl0_on = { .rst_n = 1, .pwr = 1 };
  int err;

  if ((err = stusb4500_i2c_write(stusb4500, STUSB4500_FTP_CTRL_0, &ctrl0_reset, sizeof(ctrl0_reset)))) {
    LOG_ERROR("stusb4500_i2c_write STUSB4500_FTP_CTRL_0");
    return err;
  }

  if ((err = stusb4500_i2c_write(stusb4500, STUSB4500_FTP_CTRL_0, &ctrl0_on, sizeof(ctrl0_on)))) {
    LOG_ERROR("stusb4500_i2c_write STUSB4500_FTP_CTRL_0");
    return err;
  }

  return 0;
}

static int stusb4500_nvm_op(struct stusb4500 *stusb4500, uint8_t opcode, uint8_t ser, uint8_t sector)
{
  struct stusb4500_ftp_ctrl_1 ctrl1_op = { .opcode = opcode, .ser = ser };
  struct stusb4500_ftp_ctrl_0 ctrl0_op = { .sector = sector, .req = 1, .rst_n = 1, .pwr = 1 };
  int err;

  if ((err = stusb4500_i2c_write(stusb4500, STUSB4500_FTP_CTRL_1, &ctrl1_op, sizeof(ctrl1_op)))) {
    LOG_ERROR("stusb4500_i2c_write STUSB4500_FTP_CTRL_1");
    return err;
  }

  if ((err = stusb4500_i2c_write(stusb4500, STUSB4500_FTP_CTRL_0, &ctrl0_op, sizeof(ctrl0_op)))) {
    LOG_ERROR("stusb4500_i2c_write STUSB4500_FTP_CTRL_1");
    return err;
  }

  while (ctrl0_op.req) {
    if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_FTP_CTRL_0, &ctrl0_op, sizeof(ctrl0_op)))) {
      LOG_ERROR("stusb4500_i2c_read STUSB4500_FTP_CTRL_0");
      return err;
    }
  }

  return 0;
}

static int stusb4500_nvm_read_sector(struct stusb4500 *stusb4500, uint8_t sector, stusb4500_nvm_sector_t *data)
{
  int err;

  if ((err = stusb4500_nvm_reset(stusb4500))) {
    return err;
  }

  if ((err = stusb4500_nvm_op(stusb4500, STUSB4500_FTP_CTRL_OPCODE_READ, 0, sector))) {
    return err;
  }

  if ((err = stusb4500_i2c_read(stusb4500, STUSB4500_RW_BUFFER_0, data, sizeof(*data)))) {
    LOG_ERROR("stusb4500_i2c_read STUSB4500_RW_BUFFER_0");
    return err;
  }

  return 0;
}

int stusb4500_nvm_read(struct stusb4500 *stusb4500, struct stusb4500_nvm *nvm)
{
  int err;

  if ((err = stusb4500_nvm_unlock(stusb4500))) {
    LOG_ERROR("stusb4500_nvm_unlock");
    return err;
  }

  for (int i = 0; i < STUSB4500_NVM_SECTOR_COUNT; i++) {
    if ((err = stusb4500_nvm_read_sector(stusb4500, i, &nvm->sectors[i]))) {
      LOG_ERROR("stusb4500_nvm_read_sector %d", i);
      return err;
    }
  }

  if ((err = stusb4500_nvm_lock(stusb4500))) {
    LOG_ERROR("stusb4500_nvm_lock");
    return err;
  }


  return 0;
}
