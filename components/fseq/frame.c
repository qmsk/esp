#include <fseq.h>
#include "fseq.h"

#include <stdlib.h>

#include <logging.h>

int fseq_frame_new(struct fseq_frame **framep, struct fseq *fseq)
{
  struct fseq_frame *frame;
  size_t size = fseq_get_frame_size(fseq);

  if (!(frame = malloc(sizeof(*frame) + size))) {
    LOG_ERROR("malloc %u + %u", sizeof(*frame), size);
    return -1;
  }

  frame->size = size;

  *framep = frame;

  return 0;
}
