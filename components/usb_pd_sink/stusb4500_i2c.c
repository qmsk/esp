#include "stusb4500.h"
#include "stusb4500_i2c.h"

#include <driver/i2c.h>
#include <esp_err.h>

#include <logging.h>

int stusb4500_i2c_read(struct stusb4500 *stusb4500, enum stusb4500_i2c_register cmd, void *buf, size_t size)
{
  uint8_t buffer[I2C_LINK_RECOMMENDED_SIZE(2)] = { 0 };
  i2c_cmd_handle_t handle;
  esp_err_t err;

  if (!(handle = i2c_cmd_link_create_static(buffer, sizeof(buffer)))) {
    LOG_ERROR("i2c_cmd_link_create_static");
    return -1;
  }

  if ((err = i2c_master_start(handle))) {
    LOG_ERROR("i2c_master_start: %s", esp_err_to_name(err));
    goto error;
  }

  LOG_DEBUG("i2c_addr=%02x cmd=%02x", stusb4500->i2c_addr, cmd);

  if ((err = i2c_master_write_byte(handle, stusb4500->i2c_addr << 1 | I2C_MASTER_WRITE, true))) {
    LOG_ERROR("i2c_master_write_byte: %s", esp_err_to_name(err));
    goto error;
  }

  if ((err = i2c_master_write_byte(handle, cmd, true))) {
    LOG_ERROR("i2c_master_write_byte: %s", esp_err_to_name(err));
    goto error;
  }

  if ((err = i2c_master_start(handle))) {
    LOG_ERROR("i2c_master_start: %s", esp_err_to_name(err));
    goto error;
  }

  if ((err = i2c_master_write_byte(handle, stusb4500->i2c_addr << 1 | I2C_MASTER_READ, true))) {
    LOG_ERROR("i2c_master_write_byte: %s", esp_err_to_name(err));
    goto error;
  }

  if ((err = i2c_master_read(handle, buf, size, I2C_MASTER_LAST_NACK))) {
    LOG_ERROR("i2c_master_write: %s", esp_err_to_name(err));
    goto error;
  }

  if ((err = i2c_master_stop(handle))) {
    LOG_ERROR("i2c_master_stop: %s", esp_err_to_name(err));
    goto error;
  }

  if ((err = i2c_master_cmd_begin(stusb4500->i2c_port, handle, stusb4500->i2c_timeout))) {
    LOG_ERROR("i2c_master_cmd_begin: %s", esp_err_to_name(err));
    goto error;
  }

  for (int i = 0; i < size; i++) {
    LOG_DEBUG("\t%02x", ((uint8_t *) buf)[i]);
  }

error:
  i2c_cmd_link_delete_static(handle);

  return err;
}

int stusb4500_i2c_write(struct stusb4500 *stusb4500, enum stusb4500_i2c_register cmd, const void *buf, size_t size)
{
  uint8_t buffer[I2C_LINK_RECOMMENDED_SIZE(1)] = { 0 };
  i2c_cmd_handle_t handle;
  esp_err_t err;

  if (!(handle = i2c_cmd_link_create_static(buffer, sizeof(buffer)))) {
    LOG_ERROR("i2c_cmd_link_create_static");
    return -1;
  }

  if ((err = i2c_master_start(handle))) {
    LOG_ERROR("i2c_master_start: %s", esp_err_to_name(err));
    goto error;
  }

  LOG_DEBUG("i2c_addr=%02x cmd=%02x", stusb4500->i2c_addr, cmd);

  if ((err = i2c_master_write_byte(handle, stusb4500->i2c_addr << 1 | I2C_MASTER_WRITE, true))) {
    LOG_ERROR("i2c_master_write_byte: %s", esp_err_to_name(err));
    goto error;
  }

  if ((err = i2c_master_write_byte(handle, cmd, true))) {
    LOG_ERROR("i2c_master_write_byte: %s", esp_err_to_name(err));
    goto error;
  }

  for (int i = 0; i < size; i++) {
    LOG_DEBUG("\t%02x", ((uint8_t *) buf)[i]);
  }

  if ((err = i2c_master_write(handle, buf, size, true))) {
    LOG_ERROR("i2c_master_write: %s", esp_err_to_name(err));
    goto error;
  }

  if ((err = i2c_master_stop(handle))) {
    LOG_ERROR("i2c_master_stop: %s", esp_err_to_name(err));
    goto error;
  }

  if ((err = i2c_master_cmd_begin(stusb4500->i2c_port, handle, stusb4500->i2c_timeout))) {
    LOG_ERROR("i2c_master_cmd_begin: %s", esp_err_to_name(err));
    goto error;
  }

error:
  i2c_cmd_link_delete_static(handle);

  return err;
}
