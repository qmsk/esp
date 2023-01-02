#include <fseq.h>
#include "fseq.h"

#include <stdlib.h>

#include <logging.h>

int fseq_frame_init(struct fseq_frame *frame, struct fseq *fseq)
{
  frame->size = fseq_get_frame_size(fseq);

  if (!(frame->buf = malloc(frame->size))) {
    LOG_ERROR("malloc %u", frame->size);
    return -1;
  }

  return 0;
}

size_t fseq_size(struct fseq *fseq)
{
  return fseq_get_frame_size(fseq);
}
