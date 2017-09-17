#ifndef __SPI_CONFIG_H__
#define __SPI_CONFIG_H__

enum SPI_Mode {
  SPI_MODE_CPHA   = 0x1,  // rising edge
  SPI_MODE_CPOL   = 0x2,

  SPI_MODE_0    = 0,
  SPI_MODE_1    = SPI_MODE_CPHA,
  SPI_MODE_2    = SPI_MODE_CPOL,
  SPI_MODE_3    = SPI_MODE_CPHA | SPI_MODE_CPOL,
};

enum SPI_Clock {
  SPI_CLOCK_SYS     = 0,
  SPI_CLOCK_80MHZ   = 1,
  SPI_CLOCK_40MHZ   = 2,
  SPI_CLOCK_20MHZ   = 4,
  SPI_CLOCK_10MHZ   = 8,
  SPI_CLOCK_5MHZ    = 16,
  SPI_CLOCK_1MHZ    = 80,
};

struct SPI_MasterConfig {
  enum SPI_Mode mode;
  enum SPI_Clock clock;
};

#endif
