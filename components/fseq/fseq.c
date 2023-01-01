#include "fseq.h"
#include <logging.h>

#include <stdlib.h>

int fseq_new(struct fseq **fseqp, FILE *file)
{
  struct fseq *fseq;
  int err;

  if (!(fseq = calloc(1, sizeof(*fseq)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = fseq_init_file(fseq,  file))) {
    LOG_ERROR("fseq_init_file");
    goto error;
  }

  *fseqp = fseq;

  return 0;

error:
  free(fseq);

  return err;
}
