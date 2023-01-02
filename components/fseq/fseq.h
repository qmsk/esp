#pragma once

#include <fseq.h>
#include "file.h"

#include <stdio.h>

struct fseq {
  FILE *file;

  // headers
  struct fseq_header_v2 header;
  struct fseq_compression_block *compression_blocks;
  struct fseq_sparse_range *sparse_ranges;
  struct fseq_variable_header **variable_headers;
  unsigned variable_headers_count;

  // state
  enum fseq_mode mode;
  unsigned frame;
  TickType_t tick;
};

/* file.c */
static inline unsigned fseq_get_frame_count(struct fseq *fseq)
{
  return fseq->header.frame_count;
}

static inline size_t fseq_get_frame_size(struct fseq *fseq)
{
  return fseq->header.channel_count;
}

static inline TickType_t fseq_get_frame_ticks(struct fseq *fseq)
{
  return fseq->header.frame_step_ms / portTICK_PERIOD_MS;
}

int fseq_read_headers(struct fseq *fseq);
int fseq_seek_frame(struct fseq *fseq, unsigned frame);
int fseq_read_frame(struct fseq *fseq, struct fseq_frame *frame);
