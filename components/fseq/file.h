#pragma once

#include <stdint.h>

#define FSEQ_V2_ID { 'P', 'S', 'E', 'Q' }
#define FSEQ_V2_MAJOR_VERSION 2
#define FSEQ_V2_MINOR_VERSION 0

struct __attribute__((packed)) fseq_header_v2  {
  char      id[4];
  uint16_t  data_offset;
  uint8_t   minor_version;
  uint8_t   major_version;
  uint16_t  header_length;
  uint32_t  channel_count;
  uint32_t  frame_count;
  uint8_t   frame_step_ms;
  uint8_t   flags;
  uint8_t   compression_type;
  uint8_t   compression_block_count;
  uint8_t   sparse_range_count;
  uint8_t   _reserved;
  uint64_t  unique_id;
};

struct __attribute__((packed)) fseq_compression_block {
  uint32_t frame_index;
  uint32_t length;
};

struct __attribute__((packed)) fseq_sparse_range {
  unsigned start_index : 24;
  unsigned end_offset : 24;
};

struct __attribute__((packed)) fseq_variable_header {
  uint16_t  length;
  uint16_t  code;
  uint8_t   data[];
};
