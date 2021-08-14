#ifndef __SPI_LEDS_H__
#define __SPI_LEDS_H__

#include <spi_master.h>
#include <stdint.h>

struct spi_leds;

enum spi_leds_protocol {
  SPI_LEDS_PROTOCOL_APA102          = 1,
  SPI_LEDS_PROTOCOL_P9813           = 2,
};

struct spi_leds_options {
  enum spi_leds_protocol protocol;
  enum spi_clock clock;
  unsigned count;
};

struct spi_led_color {
  uint8_t r, g, b;

  union {
    uint8_t parameter;
    uint8_t brightness; // optional, 0-255
  } parameters;
};


int spi_leds_new(struct spi_leds **spi_ledsp, struct spi_master *spi_master, const struct spi_leds_options options);

/* Get LED count */
unsigned spi_leds_count(struct spi_leds *spi_leds);

/* Get active LED count */
unsigned spi_leds_active(struct spi_leds *spi_leds);

/*
 * @param index 0-based index
 * @param global 5-bit global brightness 0-31
 * @param b, g, r 8-bit RGB value
 */
int spi_leds_set(struct spi_leds *spi_leds, unsigned index, struct spi_led_color color);

/*
 * @param global 5-bit global brightness 0-31
 * @param b, g, r 8-bit RGB value
 */
int spi_leds_set_all(struct spi_leds *spi_leds, struct spi_led_color color);

/* Send frames on SPI bus */
int spi_leds_tx(struct spi_leds *spi_leds);

#endif
