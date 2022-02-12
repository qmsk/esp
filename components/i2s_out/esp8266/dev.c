#include "../i2s_out.h"

int i2s_out_dev_setup(struct i2s_out *i2s_out, struct i2s_out_options options)
{
  // no-op, only one I2S dev which is hardcoded
  return 0;
}

void i2s_out_dev_teardown(struct i2s_out *i2s_out)
{
  
}
