#include <i2s_out.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#if CONFIG_IDF_TARGET_ESP32
# include <esp_intr_alloc.h>
# include <soc/i2s_struct.h>
#endif

struct dma_desc;

struct i2s_out {
  i2s_port_t port;
  SemaphoreHandle_t mutex;

#if CONFIG_IDF_TARGET_ESP32
  portMUX_TYPE mux;
    /* dev */
  SemaphoreHandle_t dev_mutex;
  i2s_dev_t *dev;
  intr_handle_t intr;
#endif

  /* pin */
  SemaphoreHandle_t pin_mutex;
#if CONFIG_IDF_TARGET_ESP32
  gpio_num_t data_gpio;
#endif
  /* i2s */
  // task waiting for tx_rempty notify
  xTaskHandle i2s_flush_task;

  /* dma */
  uint8_t *dma_rx_buf, *dma_eof_buf;
  struct dma_desc *dma_rx_desc;
  struct dma_desc *dma_eof_desc;

  unsigned dma_rx_count;

  // pointer to software-owned dma_rx_desc used for write()
  struct dma_desc *dma_write_desc;

  // task waiting for EOF notify
  xTaskHandle dma_flush_task;
};

/* dma.c */
int i2s_out_dma_init(struct i2s_out *i2s_out, size_t size);
int i2s_out_dma_setup(struct i2s_out *i2s_out, struct i2s_out_options options);
int i2s_out_dma_write(struct i2s_out *i2s_out, void *buf, size_t size);
int i2s_out_dma_ready(struct i2s_out *i2s_out);
void i2s_out_dma_start(struct i2s_out *i2s_out);
int i2s_out_dma_flush(struct i2s_out *i2s_out);

/* i2s.c */
int i2s_out_i2s_init(struct i2s_out *i2s_out);
int i2s_out_i2s_setup(struct i2s_out *i2s_out, struct i2s_out_options options);
void i2s_out_i2s_start(struct i2s_out *i2s_out);
int i2s_out_i2s_flush(struct i2s_out *i2s_out);

/* dev.c */
int i2s_out_dev_setup(struct i2s_out *i2s_out, struct i2s_out_options options);
void i2s_out_dev_teardown(struct i2s_out *i2s_out);

/* pin.c */
int i2s_out_pin_init(struct i2s_out *i2s_out);
int i2s_out_pin_setup(struct i2s_out *i2s_out, struct i2s_out_options options);
void i2s_out_pin_teardown(struct i2s_out *i2s_out);

/* intr.c */
int i2s_out_intr_setup(struct i2s_out *i2s_out, struct i2s_out_options options);
void i2s_out_intr_teardown(struct i2s_out *i2s_out);
