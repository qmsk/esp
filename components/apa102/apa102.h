#pragma once

struct __attribute__((packed)) apa102_frame {
  uint8_t global;
  uint8_t b, g, r;
};

struct apa102 {
  spi_host_t spi_host;
  enum apa102_protocol protocol;
  unsigned count;

  // tx
  uint8_t *buf;
  unsigned len;
  struct apa102_frame *frames;
};
