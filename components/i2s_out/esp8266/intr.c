#include "../i2s_out.h"

int i2s_out_intr_setup(struct i2s_out *i2s_out, struct i2s_out_options options)
{
  // i2s/dma have separate ISRs
  return 0;
}

void i2s_out_intr_teardown(struct i2s_out *i2s_out)
{

}
