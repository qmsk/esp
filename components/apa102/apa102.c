#include <apa102.h>
#include "apa102.h"

#include <logging.h>

#include <esp_err.h>
#include <stdlib.h>
#include <string.h>

#define APA102_SPI_CPOL 0
#define APA102_SPI_CPHA 0
#define APA102_SPI_BIT_ORDER 0
#define APA102_SPI_BYTE_ORDER 0

// 512 bits = 64 bytes
#define SPI_TRANS_SIZE 64

int apa102_init(struct apa102 *apa102, const struct apa102_options *options)
{
  spi_config_t spi_config = {
    .interface.cpol = APA102_SPI_CPOL,
    .interface.cpha = APA102_SPI_CPHA,
    .interface.bit_tx_order = APA102_SPI_BIT_ORDER,
    .interface.bit_rx_order = APA102_SPI_BIT_ORDER,
    .interface.byte_tx_order = APA102_SPI_BYTE_ORDER,
    .interface.byte_rx_order = APA102_SPI_BYTE_ORDER,
    .interface.mosi_en = 1,
    .interface.miso_en = 0,
    .interface.cs_en = 0,

    .mode = SPI_MASTER_MODE,

    .clk_div = options->spi_clk_div,
  };
  esp_err_t err;

  if ((err = spi_init(options->spi_host, &spi_config))) {
    LOG_ERROR("spi_init: %s", esp_err_to_name(err));
    return -1;
  }

  apa102->spi_host = options->spi_host;
  apa102->protocol = options->protocol;
  apa102->count = options->count;

  return 0;
}

int apa102_init_tx(struct apa102 *apa102)
{
  uint8_t stopbyte = (apa102->protocol & APA102_PROTOCOL_STOP_ZEROBITS) ? 0x00 : 0xff;
  unsigned stopbytes = 4 * (1 + apa102->count / 32); // one bit per LED, in frames of 32 bits

  apa102->len = (1 + apa102->count) * sizeof(struct apa102_frame) + stopbytes;

  LOG_INFO("count=%u len=%d stopbyte=%02x stopbytes=%u", apa102->count, apa102->len, stopbyte, stopbytes);

  if ((apa102->buf = malloc(apa102->len)) == NULL) {
    LOG_ERROR("malloc");
    return -1;
  }

  memset(apa102->buf + 0, 0x00, 4); // 32 start bits (zero)
  memset(apa102->buf + (1 + apa102->count) * sizeof(struct apa102_frame), stopbyte, stopbytes); // stop bits

  // frames
  apa102->frames = (struct apa102_frame *)(apa102->buf) + 1;

  for (unsigned i = 0; i < apa102->count; i++) {
    apa102->frames[i] = (struct apa102_frame){ 0xE0, 0, 0, 0 }; // zero
  }

  return 0;
}

int apa102_new(struct apa102 **apa102p, const struct apa102_options *options)
{
  struct apa102 *apa102;
  int err;

  if (!(apa102 = calloc(1, sizeof(*apa102)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = apa102_init(apa102, options))) {
    LOG_ERROR("apa102_init");
    goto error;
  }

  if ((err = apa102_init_tx(apa102))) {
    LOG_ERROR("apa102_init");
    goto error;
  }

  *apa102p = apa102;

  return 0;

error:
  free(apa102);

  return err;
}

unsigned apa102_count(struct apa102 *apa102)
{
  return apa102->count;
}


int apa102_set(struct apa102 *apa102, unsigned index, uint8_t global, uint8_t b, uint8_t g, uint8_t r)
{
  struct apa102_frame frame = {
    .global = 0xE0 | (global & 0x1F),
    .b = b,
    .g = g,
    .r = r,
  };

  LOG_DEBUG("[%03d] %02x:%02x%02x%02x", index, frame.global, frame.b, frame.g, frame.r);

  if (index >= apa102->count) {
    LOG_WARN("index %u >= count %u", index, apa102->count);
    return -1;
  }

  apa102->frames[index] = frame;

  return 0;
}

int apa102_set_all(struct apa102 *apa102, uint8_t global, uint8_t b, uint8_t g, uint8_t r)
{
  struct apa102_frame frame = {
    .global = 0xE0 | (global & 0x1F),
    .b = b,
    .g = g,
    .r = r,
  };

  LOG_DEBUG("[%03d] %02x:%02x%02x%02x", apa102->count, frame.global, frame.b, frame.g, frame.r);

  for (struct apa102_frame *f = apa102->frames; f < apa102->frames + apa102->count; f++) {
    *f = frame;
  }

  return 0;
}

int apa102_tx(struct apa102 *apa102)
{
  uint8_t *buf = apa102->buf;
  unsigned len = apa102->len;
  esp_err_t err;

  while (len) {
    unsigned size = len > SPI_TRANS_SIZE ? SPI_TRANS_SIZE : len;

    // buf, len and size must always be 32-bit aligned
    spi_trans_t trans = {
      .mosi = (uint32_t *) buf,
      .bits.mosi = size * 8,
    };

    if ((err = spi_trans(apa102->spi_host, &trans))) {
      LOG_ERROR("spi_trans: %s", esp_err_to_name(err));
      return -1;
    }

    buf += size;
    len -= size;
  }

  return 0;
}
