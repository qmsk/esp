#pragma once

#include "i2c.h"

#include <sdkconfig.h>

#if CONFIG_I2C_MASTER_ENABLED
  #include <driver/i2c.h>

  #ifdef I2C_NUM_0
    #define I2C_MASTER_PORT I2C_NUM_0
  #endif
#endif
