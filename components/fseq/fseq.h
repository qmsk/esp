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
};

/* file.c */
int fseq_init_file(struct fseq *fseq, FILE *file);
