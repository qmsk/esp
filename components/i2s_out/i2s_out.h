#include <i2s_out.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#if CONFIG_IDF_TARGET_ESP32
# include <esp_intr_alloc.h>
# include <soc/i2s_struct.h>
#endif

struct dma_desc;

#define I2S_OUT_EVENT_GROUP_BIT_DMA_EOF (1 << 0)
#define I2S_OUT_EVENT_GROUP_BIT_I2S_EOF (1 << 1)

struct i2s_out {
  i2s_port_t port;
  SemaphoreHandle_t mutex;
#if CONFIG_IDF_TARGET_ESP32
  portMUX_TYPE mux;
#endif
  EventGroupHandle_t event_group;

  /* dev */
  SemaphoreHandle_t dev_mutex;
#if CONFIG_IDF_TARGET_ESP32
  i2s_dev_t *dev;
  intr_handle_t intr;
#endif

  /* pin */
  SemaphoreHandle_t pin_mutex;
#if CONFIG_IDF_TARGET_ESP32
  bool bck_gpio_inv;
  gpio_num_t bck_gpios[I2S_OUT_GPIO_PINS_MAX];
  gpio_num_t data_gpios[I2S_OUT_GPIO_PINS_MAX];
  gpio_num_t inv_data_gpios[I2S_OUT_GPIO_PINS_MAX];
#endif

  /* dma */
  uint8_t *dma_out_buf, *dma_end_buf;
  struct dma_desc *dma_out_desc;
  struct dma_desc *dma_repeat_desc;
  struct dma_desc *dma_end_desc;

  unsigned dma_out_count, dma_repeat_count;

  // pointer to software-owned dma_out_desc used for write()
  struct dma_desc *dma_write_desc;

  bool dma_start; // set by i2s_out_dma_start
};

/* dma.c */
int i2s_out_dma_init(struct i2s_out *i2s_out, size_t size, size_t align, unsigned repeat);
int i2s_out_dma_setup(struct i2s_out *i2s_out, const struct i2s_out_options *options);
size_t i2s_out_dma_buffer(struct i2s_out *i2s_out, void **ptr, unsigned count, size_t size);
void i2s_out_dma_commit(struct i2s_out *i2s_out, unsigned count, size_t size);
int i2s_out_dma_write(struct i2s_out *i2s_out, const void *data, size_t size);
void i2s_out_dma_repeat(struct i2s_out *i2s_out, unsigned count);
int i2s_out_dma_pending(struct i2s_out *i2s_out);
void i2s_out_dma_start(struct i2s_out *i2s_out);
int i2s_out_dma_flush(struct i2s_out *i2s_out);
void i2s_out_dma_free(struct i2s_out *i2s_out);

/* i2s.c */
int i2s_out_i2s_init(struct i2s_out *i2s_out);
int i2s_out_i2s_setup(struct i2s_out *i2s_out, const struct i2s_out_options *options);
void i2s_out_i2s_start(struct i2s_out *i2s_out);
int i2s_out_i2s_flush(struct i2s_out *i2s_out);
void i2s_out_i2s_stop(struct i2s_out *i2s_out);

/* dev.c */
int i2s_out_dev_setup(struct i2s_out *i2s_out, const struct i2s_out_options *options);
void i2s_out_dev_teardown(struct i2s_out *i2s_out);

/* pin.c */
int i2s_out_pin_init(struct i2s_out *i2s_out);
int i2s_out_pin_setup(struct i2s_out *i2s_out, const struct i2s_out_options *options);
void i2s_out_pin_teardown(struct i2s_out *i2s_out);

/* intr.c */
int i2s_out_intr_setup(struct i2s_out *i2s_out, const struct i2s_out_options *options);
void i2s_out_intr_teardown(struct i2s_out *i2s_out);
