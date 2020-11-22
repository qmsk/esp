#include "p9813.h"
#include "p9813_config.h"
#include "user_cmd.h"
#include "spi.h"
#include "artnet_dmx.h"

#include <lib/logging.h>

#include <stdlib.h>
#include <drivers/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define P9813_TASK_STACK 512

static const enum SPI_Mode p9813_spi_mode = SPI_MODE_0;
static const enum SPI_Clock p9813_spi_clock = SPI_CLOCK_1MHZ;

struct __attribute__((packed)) p9813_packet {
  uint8_t control;
  uint8_t r, g, b;
};

void p9813_packet_set(struct p9813_packet *packet, uint8_t b, uint8_t g, uint8_t r)
{
  packet->control = 0xC0 | ((~b & 0xC0) >> 2) | ((~g & 0xC0) >> 4) | ((~r & 0xC0) >> 6);
  packet->b = b;
  packet->g = g;
  packet->r = r;
}

struct p9813 {
  struct spi *spi;
  enum GPIO gpio;

  unsigned count;

  bool active;
  struct p9813_packet *buf;

  xQueueHandle artnet_queue;
  xTaskHandle artnet_task;
  struct artnet_dmx artnet_dmx;
} p9813;

int p9813_init_spi(struct p9813 *p9813, struct spi *spi)
{
  struct SPI_MasterConfig spi_config = {
    .mode   = p9813_spi_mode,
    .clock  = p9813_spi_clock,
  };
  int err;

  p9813->spi = spi;

  if ((err = spi_setup(spi, spi_config))) {
    LOG_ERROR("spi_setup");
    return -1;
  }

  LOG_INFO("mode=%u clock=%u", spi_config.mode, spi_config.clock);

  return 0;
}

int p9813_init_gpio(struct p9813 *p9813, unsigned gpio)
{
  if (!GPIO_Exists(gpio)) {
    LOG_ERROR("invalid gpio=%u", gpio);
    return -1;
  }

  p9813->gpio = gpio;

  LOG_INFO("gpio=%u", p9813->gpio);

  GPIO_SetupOutput(p9813->gpio, GPIO_OUTPUT_LOW);

  return 0;
}

int p9813_init_tx(struct p9813 *p9813, unsigned count)
{
  p9813->count = count;

  if ((p9813->buf = malloc((1 + p9813->count + 1) * sizeof(*p9813->buf))) == NULL) {
    LOG_ERROR("malloc");
    return -1;
  }

  LOG_INFO("count=%u", count);

  p9813->buf[0] = (struct p9813_packet){ 0, 0, 0, 0 }; // start

  for (unsigned i = 0; i < p9813->count; i++) {
    p9813->buf[1 + i] = (struct p9813_packet){ 0xFF, 0, 0, 0 }; // zero
  }

  p9813->buf[p9813->count + 1] = (struct p9813_packet){ 0, 0, 0, 0 }; // stop

  return 0;
}

int p9813_init_artnet(struct p9813 *p9813, uint16_t artnet_universe)
{
  int err;

  if ((p9813->artnet_queue = xQueueCreate(1, sizeof(struct artnet_dmx))) == NULL) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  LOG_INFO("universe=%u", artnet_universe);

  if ((err = start_artnet_output(artnet_universe, p9813->artnet_queue))) {
    LOG_ERROR("patch_artnet_output");
    return err;
  }

  return 0;
}

int p9813_init(struct p9813 *p9813, const struct p9813_config *config, struct spi *spi)
{
  int err;

  if ((err = p9813_init_spi(p9813, spi)))
    return err;

  if (config->gpio) {
    if ((err = p9813_init_gpio(p9813, config->gpio)))
      return err;
  }

  if ((err = p9813_init_tx(p9813, config->count)))
    return err;

  if (config->artnet_universe) {
    if ((err = p9813_init_artnet(p9813, config->artnet_universe)))
      return err;
  }

  return 0;
}

void p9813_off(struct p9813 *p9813)
{
  LOG_INFO("gpio=%u low", p9813->gpio);

  GPIO_OutputLow(p9813->gpio);

  p9813->active = false;
}

void p9813_on(struct p9813 *p9813)
{
  LOG_INFO("gpio=%u high", p9813->gpio);

  GPIO_OutputHigh(p9813->gpio);

  p9813->active = true;
}

void p9813_inactive(struct p9813 *p9813)
{
  if (p9813->active) {
    // TODO: timer?
    p9813_off(p9813);
  }
}

void p9813_active(struct p9813 *p9813)
{
  if (!p9813->active) {
    p9813_on(p9813);
  }
}

int p9813_tx(struct p9813 *p9813)
{
  const char *ptr = (void *) p9813->buf;
  size_t len = (1 + p9813->count + 1) * sizeof(*p9813->buf);
  int ret;

  while (len > 0) {
    if ((ret = spi_write(p9813->spi, ptr, len)) < 0) {
      LOG_ERROR("spi_write");
      return ret;
    } else {
      ptr += ret;
      len -= ret;
    }
  }

  return 0;
}

int p9813_artnet_dmx(struct p9813 *p9813, const struct artnet_dmx *dmx)
{
  bool active = false;

  LOG_DEBUG("len=%u", dmx->len);

  for (unsigned i = 0; i < p9813->count && dmx->len >= (i + 1) * 3; i++) {
    struct p9813_packet *packet = &p9813->buf[1 + i];

    p9813_packet_set(packet,
      dmx->data[i * 3 + 2], // b
      dmx->data[i * 3 + 1], // g
      dmx->data[i * 3 + 0]  // r
    );

    if (packet->r || packet->g || packet->b)
      active = true;
  }

  if (active) {
    p9813_active(p9813);
  } else {
    p9813_inactive(p9813);
  }

  return p9813_tx(p9813);
}

void p9813_artnet_task(void *arg)
{
  struct p9813 *p9813 = arg;
  int err;

  LOG_DEBUG("p9813=%p", p9813);

  for (;;) {
    if (!xQueueReceive(p9813->artnet_queue, &p9813->artnet_dmx, portMAX_DELAY)) {
      LOG_WARN("xQueueReceive");
    } else if ((err = p9813_artnet_dmx(p9813, &p9813->artnet_dmx))) {
      LOG_WARN("p9813_dmx");
    }
  }
}

int init_p9813 (const struct user_config *config)
{
  int err;

  if ((err = p9813_init(&p9813, &config->p9813, &spi1)))
    return err;

  if (!p9813.artnet_queue) {

  } else if ((err = xTaskCreate(&p9813_artnet_task, (signed char *) "p9813-artnet", P9813_TASK_STACK, &p9813, tskIDLE_PRIORITY + 2, &p9813.artnet_task)) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_DEBUG("p9813-artnet task=%p", p9813.artnet_task);
  }

  return 0;
}

int p9813_cmd_set(int argc, char **argv, void *ctx)
{
  struct p9813 *p9813 = ctx;
  unsigned index, rgb;
  int err;

  if ((err = cmd_arg_uint(argc, argv, 1, &index)))
    return err;
  if ((err = cmd_arg_uint(argc, argv, 2, &rgb)))
    return err;

  if (index >= p9813->count) {
    LOG_ERROR("index out of bounds");
    return -CMD_ERR_ARGV;
  }

  p9813_packet_set(&p9813->buf[1 + index], (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, (rgb >> 0) & 0xFF);
  p9813_active(p9813);

  return p9813_tx(p9813);
}

int p9813_cmd_off(int argc, char **argv, void *ctx)
{
  struct p9813 *p9813 = ctx;

  if (argc > 1)
    return -CMD_ERR_ARGC;

  p9813_off(p9813);

  return 0;
}

const struct cmd p9813_commands[] = {
  { "set",  p9813_cmd_set,  &p9813, .usage = "INDEX RGB", .describe = "Set values" },
  { "off",  p9813_cmd_off,  &p9813, .usage = "", .describe = "Power off" },
  { }
};

const struct cmdtab p9813_cmdtab = {
  .commands = p9813_commands,
};
