#ifndef __APA102_H__
#define __APA102_H__

#include <driver/spi.h>

struct apa102;

enum apa102_protocol {
  /* Send stop frame as 0-bits, standard is 1-bits */
  APA102_PROTOCOL_STOP_ZEROBITS   = 0x1,

  APA102_PROTOCOL_STANDARD        = 0,
  APA102_PROTOCOL_COMPAT          = APA102_PROTOCOL_STOP_ZEROBITS,
};

struct apa102_options {
  spi_host_t spi_host;
  spi_clk_div_t spi_clk_div;

  enum apa102_protocol protocol;
  unsigned count;
};


int apa102_new(struct apa102 **apa102p, const struct apa102_options *options);

/* Get LED count */
unsigned apa102_count(struct apa102 *apa102);

/*
 * @param index 0-based index
 * @param global 5-bit global brightness 0-31
 * @param b, g, r 8-bit RGB value
 */
int apa102_set(struct apa102 *apa102, unsigned index, uint8_t global, uint8_t b, uint8_t g, uint8_t r);

/*
 * @param global 5-bit global brightness 0-31
 * @param b, g, r 8-bit RGB value
 */
int apa102_set_all(struct apa102 *apa102, uint8_t global, uint8_t b, uint8_t g, uint8_t r);

/* Send frames on SPI bus */
int apa102_tx(struct apa102 *apa102);

#endif
