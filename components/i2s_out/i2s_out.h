#include <i2s_out.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

struct slc_desc;

struct i2s_out {
  SemaphoreHandle_t pin_mutex;
  SemaphoreHandle_t mutex;

  /* i2s */
  // task waiting for tx_rempty notify
  xTaskHandle i2s_flush_task;

  /* slc */
  uint8_t *slc_rx_buf, *slc_eof_buf;
  struct slc_desc *slc_rx_desc;
  struct slc_desc *slc_eof_desc;

  unsigned slc_rx_count;

  // pointer to software-owned slc_rx_desc used for write()
  struct slc_desc *slc_write_desc;

  // task waiting for EOF notify
  xTaskHandle slc_flush_task;
};

/* slc.c */
int i2s_out_slc_init(struct i2s_out *i2s_out, size_t size);
int i2s_out_slc_setup(struct i2s_out *i2s_out, struct i2s_out_options options);
int i2s_out_slc_write(struct i2s_out *i2s_out, void *buf, size_t size);
int i2s_out_slc_ready(struct i2s_out *i2s_out);
void i2s_out_slc_start(struct i2s_out *i2s_out);
int i2s_out_slc_flush(struct i2s_out *i2s_out);

/* i2s.c */
int i2s_out_i2s_init(struct i2s_out *i2s_out);
void i2s_out_i2s_setup(struct i2s_out *i2s_out, struct i2s_out_options options);
void i2s_out_i2s_start(struct i2s_out *i2s_out);
int i2s_out_i2s_flush(struct i2s_out *i2s_out);

/* pin.c */
int i2s_out_pin_setup(struct i2s_out *i2s_out, struct i2s_out_options options);
void i2s_out_pin_teardown(struct i2s_out *i2s_out);
