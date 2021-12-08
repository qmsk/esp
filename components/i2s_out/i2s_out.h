#include <i2s_out.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct slc_desc;

struct i2s_out {
  SemaphoreHandle_t mutex;

  /* slc */
  uint8_t *slc_rx_buf;
  struct slc_desc *slc_rx_desc;
  struct slc_desc *slc_write_desc;
};

/* slc.c */
int i2s_out_slc_init(struct i2s_out *i2s_out, size_t size);
void i2s_out_slc_setup(struct i2s_out *i2s_out, struct i2s_out_options options);
int i2s_out_slc_write(struct i2s_out *i2s_out, void *buf, size_t size);
void i2s_out_slc_start(struct i2s_out *i2s_out);

/* i2s.c */
void i2s_out_i2s_setup(struct i2s_out *i2s_out, struct i2s_out_options options);
void i2s_out_i2s_start(struct i2s_out *i2s_out);
