#include <spi_leds.h>
#include "spi_leds.h"
#include "apa102.h"
#include "p9813.h"

#include <logging.h>

#include <esp_err.h>
#include <stdlib.h>
#include <string.h>

#define SPI_LEDS_SPI_CPOL 0
#define SPI_LEDS_SPI_CPHA 0
#define SPI_LEDS_SPI_BIT_ORDER 0
#define SPI_LEDS_SPI_BYTE_ORDER 0

// 512 bits = 64 bytes
#define SPI_TRANS_SIZE 64

int spi_leds_init(struct spi_leds *spi_leds, const struct spi_leds_options *options)
{
  spi_config_t spi_config = {
    .interface.cpol = SPI_LEDS_SPI_CPOL,
    .interface.cpha = SPI_LEDS_SPI_CPHA,
    .interface.bit_tx_order = SPI_LEDS_SPI_BIT_ORDER,
    .interface.bit_rx_order = SPI_LEDS_SPI_BIT_ORDER,
    .interface.byte_tx_order = SPI_LEDS_SPI_BYTE_ORDER,
    .interface.byte_rx_order = SPI_LEDS_SPI_BYTE_ORDER,
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

  spi_leds->spi_host = options->spi_host;
  spi_leds->protocol = options->protocol;
  spi_leds->count = options->count;

  switch(options->protocol) {
    case SPI_LEDS_PROTOCOL_APA102:
      return apa102_new_packet(&spi_leds->packet.apa102, &spi_leds->packet_size, options->count);

    default:
      LOG_ERROR("unknown protocol=%#x", options->protocol);
      return -1;
  }
}

int spi_leds_new(struct spi_leds **spi_ledsp, const struct spi_leds_options *options)
{
  struct spi_leds *spi_leds;
  int err;

  if (!(spi_leds = calloc(1, sizeof(*spi_leds)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = spi_leds_init(spi_leds, options))) {
    LOG_ERROR("spi_leds_init");
    free(spi_leds);
    return -1;
  }

  *spi_ledsp = spi_leds;

  return 0;
}

unsigned spi_leds_count(struct spi_leds *spi_leds)
{
  return spi_leds->count;
}

int spi_leds_set(struct spi_leds *spi_leds, unsigned index, struct spi_led_color color)
{
  LOG_DEBUG("[%03d] %02x:%02x%02x%02x", index, color.parameters.parameter, color.r, color.g, color.b);

  if (index >= spi_leds->count) {
    LOG_WARN("index %u >= count %u", index, spi_leds->count);
    return -1;
  }

  switch(spi_leds->protocol) {
    case SPI_LEDS_PROTOCOL_APA102:
      apa102_set_frame(spi_leds->packet.apa102, index, color);
      return 0;

    default:
      LOG_ERROR("unknown protocol=%#x", spi_leds->protocol);
      return -1;
  }
}

int spi_leds_set_all(struct spi_leds *spi_leds, struct spi_led_color color)
{
  LOG_DEBUG("[%03d] %02x:%02x%02x%02x", spi_leds->count, color.parameters.parameter, color.r, color.g, color.b);

  switch(spi_leds->protocol) {
    case SPI_LEDS_PROTOCOL_APA102:
      apa102_set_frames(spi_leds->packet.apa102, spi_leds->count, color);
      return 0;

    default:
      LOG_ERROR("unknown protocol=%#x", spi_leds->protocol);
      return -1;
  }
}

int spi_leds_tx(struct spi_leds *spi_leds)
{
  uint8_t *buf = spi_leds->packet.buf;
  unsigned len = spi_leds->packet_size;
  esp_err_t err;

  while (len) {
    // esp8266 SPI transmits up to 512-bit/16-byte blocks
    unsigned trans_size = len > SPI_TRANS_SIZE ? SPI_TRANS_SIZE : len;

    // buf, len and size must always be 32-bit aligned
    spi_trans_t trans = {
      .mosi = (uint32_t *) buf,
      .bits.mosi = trans_size * 8,
    };

    if ((err = spi_trans(spi_leds->spi_host, &trans))) {
      LOG_ERROR("spi_trans: %s", esp_err_to_name(err));
      return -1;
    }

    buf += trans_size;
    len -= trans_size;
  }

  return 0;
}
