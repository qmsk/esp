#define DEBUG

/*
 * APA102 LED
 *
 * @see https://cpldcpu.wordpress.com/2014/08/27/apa102/
 * @see https://cpldcpu.wordpress.com/2014/11/30/understanding-the-apa102-superled/
 */
#include "apa102.h"

#include "artnet_dmx.h"
#include "spi.h"

#include <lib/logging.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stdlib.h>
#include <string.h>

static const enum SPI_Mode apa102_spi_mode = SPI_MODE_0;
static const enum SPI_Clock apa102_spi_clock = SPI_CLOCK_1MHZ;

enum apa102_stopbyte {
  APA102_STOP_STANDARD  = 0xff,

  // Some APA102 LEDs seem to require non-standard 0-stopbits to work correctly?
  APA102_STOP_ALTERNATE = 0x00,
};

struct __attribute__((packed)) apa102_frame {
  uint8_t global;
  uint8_t b, g, r;
};

struct apa102_config apa102_config = {

};

const struct configtab apa102_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .value  = { .boolean = &apa102_config.enabled },
  },
  { CONFIG_TYPE_BOOL, "stop_quirk",
    .value  = { .boolean = &apa102_config.stop_quirk },
  },
  { CONFIG_TYPE_UINT16, "count",
    .value  = { .uint16 = &apa102_config.count },
  },
  { CONFIG_TYPE_BOOL, "artnet_enabled",
    .value  = { .boolean = &apa102_config.artnet_enabled },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe",
    .value  = { .uint16 = &apa102_config.artnet_universe },
  },
  {}
};

#define APA102_ARTNET_TASK_STACK 512
#define APA102_ARTNET_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

struct apa102 {
  struct spi *spi;

  // tx
  unsigned count, len;
  uint8_t *buf;
  struct apa102_frame *frames;

  // artnet DMX
  xQueueHandle artnet_queue;
  xTaskHandle artnet_task;
  struct artnet_dmx artnet_dmx;

} apa102;

int apa102_init_spi(struct apa102 *apa102, struct spi *spi)
{
  struct SPI_MasterConfig spi_config = {
    .mode   = apa102_spi_mode,
    .clock  = apa102_spi_clock,
  };
  int err;

  apa102->spi = spi;

  if ((err = spi_setup(spi, spi_config))) {
    LOG_ERROR("spi_setup");
    return -1;
  }

  LOG_INFO("mode=%u clock=%u", spi_config.mode, spi_config.clock);

  return 0;
}

int apa102_init_tx(struct apa102 *apa102, unsigned count, enum apa102_stopbyte stopbyte)
{
  unsigned stopbytes = 4 * (1 + count/32); // one bit per LED, in frames of 32 bits

  apa102->count = count;
  apa102->len = (1 + count) * sizeof(struct apa102_frame) + stopbytes;

  LOG_INFO("count=%u len=%d stopbyte=%02x stopbytes=%u", apa102->count, apa102->len, stopbyte, stopbytes);

  if ((apa102->buf = malloc(apa102->len)) == NULL) {
    LOG_ERROR("malloc");
    return -1;
  }

  memset(apa102->buf + 0, 0x00, 4); // 32 start bits (zero)
  memset(apa102->buf + (1 + count) * sizeof(struct apa102_frame), stopbyte, stopbytes); // stop bits

  // frames
  apa102->frames = (struct apa102_frame *)(apa102->buf) + 1;

  for (unsigned i = 0; i < apa102->count; i++) {
    apa102->frames[i] = (struct apa102_frame){ 0xE0, 0, 0, 0 }; // zero
  }

  return 0;
}

/*
 * @param index 0-based index
 * @param global 5-bit global brightness 0-31
 * @param b, g, r 8-bit RGB value
 */
void apa102_set(struct apa102 *apa102, unsigned index, uint8_t global, uint8_t b, uint8_t g, uint8_t r)
{
  struct apa102_frame *frame;

  if (index >= apa102->count) {
    return;
  }

  frame = &apa102->frames[index];
  frame->global = 0xE0 | (global & 0x1F);
  frame->b = b;
  frame->g = g;
  frame->r = r;

  LOG_DEBUG("[%03d] %02x:%02x%02x%02x", index, frame->global, frame->b, frame->g, frame->r);
}

int apa102_tx(struct apa102 *apa102)
{
  const char *ptr = (void *) apa102->buf;
  size_t len = apa102->len;
  int ret;

  while (len > 0) {
    if ((ret = spi_write(apa102->spi, ptr, len)) < 0) {
      LOG_ERROR("spi_write");
      return ret;
    } else {
      ptr += ret;
      len -= ret;
    }
  }

  return 0;
}

int apa102_artnet_dmx(struct apa102 *apa102, const struct artnet_dmx *dmx)
{
  LOG_DEBUG("len=%u", dmx->len);

  for (unsigned i = 0; i < apa102->count && dmx->len >= (i + 1) * 3; i++) {
    apa102_set(apa102, i,
      0x1F,                 // global
      dmx->data[i * 3 + 0], // b
      dmx->data[i * 3 + 1], // g
      dmx->data[i * 3 + 2]  // r
    );
  }

  return apa102_tx(apa102);
}

void apa102_artnet_task(void *arg)
{
  struct apa102 *apa102 = arg;
  int err;

  LOG_DEBUG("apa102=%p", apa102);

  for (;;) {
    if (!xQueueReceive(apa102->artnet_queue, &apa102->artnet_dmx, portMAX_DELAY)) {
      LOG_WARN("xQueueReceive");
    } else if ((err = apa102_artnet_dmx(apa102, &apa102->artnet_dmx))) {
      LOG_WARN("apa102_artnet_dmx");
    }
  }
}

int apa102_init_artnet(struct apa102 *apa102, uint16_t artnet_universe)
{
  int err;

  LOG_INFO("artnet_universe=%u", artnet_universe);

  if ((apa102->artnet_queue = xQueueCreate(1, sizeof(struct artnet_dmx))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  if ((err = xTaskCreate(&apa102_artnet_task, (signed char *) "apa102-artnet", APA102_ARTNET_TASK_STACK, apa102, APA102_ARTNET_TASK_PRIORITY, &apa102->artnet_task)) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_DEBUG("apa102-artnet task=%p", apa102->artnet_task);
  }

  if ((err = start_artnet_output(artnet_universe, apa102->artnet_queue))) {
    LOG_ERROR("start_artnet_output");
    return err;
  }

  return 0;
}

int apa102_init(struct apa102 *apa102, const struct apa102_config *config, struct spi *spi)
{
  int err;

  if ((err = apa102_init_spi(apa102, spi))) {
    LOG_ERROR("apa102_init_spi");
    return err;
  }

  if ((err = apa102_init_tx(apa102, config->count, config->stop_quirk ? APA102_STOP_ALTERNATE : APA102_STOP_STANDARD))) {
    LOG_ERROR("apa102_init_tx");
    return err;
  }

  if (config->artnet_enabled) {
    if ((err = apa102_init_artnet(apa102, config->artnet_universe))) {
      LOG_ERROR("apa102_init_artnet");
      return err;
    }
  }

  return 0;
}

int init_apa102(const struct apa102_config *config)
{
  int err;

  if (!config->enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  if ((err = apa102_init(&apa102, config, &spi1))) {
    LOG_ERROR("apa102_init");
    return err;
  }

  return 0;
}

int apa102_cmd_clear(int argc, char **argv, void *ctx)
{
  struct apa102 *apa102 = ctx;
  int err;

  for (unsigned i = 0; i < apa102->count; i++) {
    apa102_set(apa102, i, 0, 0, 0, 0);
  }

  if ((err = apa102_tx(apa102))) {
    LOG_ERROR("apa102_tx");
    return err;
  }

  return 0;
}

int apa102_cmd_all(int argc, char **argv, void *ctx)
{
  struct apa102 *apa102 = ctx;
  int rgb, a = 0xff;
  int err;

  if ((err = cmd_arg_int(argc, argv, 1, &rgb)))
    return err;
  if ((argc > 2) && (err = cmd_arg_int(argc, argv, 2, &a)))
    return err;

  for (unsigned index = 0; index < apa102->count; index++) {
    apa102_set(apa102, index,
      a >> 3,             // a
      (rgb >> 0)  & 0xFF, // b
      (rgb >> 8)  & 0xFF, // g
      (rgb >> 16) & 0xFF  // r
    );
  }

  if ((err = apa102_tx(apa102))) {
    LOG_ERROR("apa102_tx");
    return err;
  }

  return 0;
}

int apa102_cmd_set(int argc, char **argv, void *ctx)
{
  struct apa102 *apa102 = ctx;
  unsigned index;
  int rgb, a = 0xff;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &index)))
    return err;
  if ((err = cmd_arg_int(argc, argv, 2, &rgb)))
    return err;
  if ((argc > 3) && (err = cmd_arg_int(argc, argv, 3, &a)))
    return err;

  if (index >= apa102->count) {
    LOG_ERROR("index out of bounds");
    return -CMD_ERR_ARGV;
  }

  apa102_set(apa102, index,
    a >> 3,             // a
    (rgb >> 0)  & 0xFF, // b
    (rgb >> 8)  & 0xFF, // g
    (rgb >> 16) & 0xFF  // r
  );

  if ((err = apa102_tx(apa102))) {
    LOG_ERROR("apa102_tx");
    return err;
  }

  return 0;
}

const struct cmd apa102_commands[] = {
  { "clear",  apa102_cmd_clear, &apa102, .usage = "",               .describe = "Clear values" },
  { "all",    apa102_cmd_all,   &apa102, .usage = "RGB [A]",        .describe = "Set all pixels to values" },
  { "set",    apa102_cmd_set,   &apa102, .usage = "INDEX RGB [A]",  .describe = "Set one pixel to value" },
  { }
};

const struct cmdtab apa102_cmdtab = {
  .commands = apa102_commands,
};
