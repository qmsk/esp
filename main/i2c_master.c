#include "i2c_master.h"
#include "i2c_config.h"

#include <logging.h>

int init_i2c_master()
{
#if CONFIG_I2C_MASTER_ENABLED
  i2c_config_t config = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num     = CONFIG_I2C_MASTER_SDA_IO_NUM,
    .scl_io_num     = CONFIG_I2C_MASTER_SCL_IO_NUM,
  #if CONFIG_I2C_MASTER_INTERNAL_PULLUP_ENABLED
    .sda_pullup_en  = true,
    .scl_pullup_en  = true,
  #else
    .sda_pullup_en  = false,
    .scl_pullup_en  = false,
  #endif
    .master.clk_speed = CONFIG_I2C_MASTER_CLK_SPEED,
  };
  size_t slv_rx_buf_len = 0, slv_tx_buf_len = 0;
  int intr_alloc_flags = 0;
  esp_err_t err;

  LOG_INFO("port=%d: sda_io_num=%d scl_io_num=%d sda_pullup_en=%d scl_pullup_en=%d clk_speed=%u", I2C_MASTER_PORT,
    config.sda_io_num,
    config.scl_io_num,
    config.sda_pullup_en,
    config.scl_pullup_en,
    config.master.clk_speed
  );

  if ((err = i2c_param_config(I2C_MASTER_PORT, &config))) {
    LOG_ERROR("i2c_param_config: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = i2c_driver_install(I2C_MASTER_PORT, I2C_MODE_MASTER, slv_rx_buf_len, slv_tx_buf_len, intr_alloc_flags))) {
    LOG_ERROR("i2c_driver_install: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;

#else
  LOG_INFO("disabled");

  return 0;
#endif
}
