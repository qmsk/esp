#include "fseq.h"
#include <logging.h>

#include <stdlib.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

int fseq_init(struct fseq *fseq, FILE *file)
{
  int err;

  fseq->file = file;

  if ((err = fseq_read_headers(fseq))) {
    LOG_ERROR("fseq_init_file");
    return err;
  }

  return 0;
}

int fseq_new(struct fseq **fseqp, FILE *file)
{
  struct fseq *fseq;
  int err;

  if (!(fseq = calloc(1, sizeof(*fseq)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = fseq_init(fseq,  file))) {
    LOG_ERROR("fseq_init");
    goto error;
  }

  *fseqp = fseq;

  return 0;

error:
  free(fseq);

  return err;
}

enum fseq_state fseq_state(struct fseq *fseq)
{
  if (fseq->tick) {
    return FSEQ_STATE_PLAY;
  } else {
    return FSEQ_STATE_STOP;
  }
}

int fseq_start(struct fseq *fseq, enum fseq_mode mode)
{
  int err;

  fseq->mode = mode;
  fseq->frame = 0;
  fseq->tick = xTaskGetTickCount();

  if ((err = fseq_seek_frame(fseq, fseq->frame))) {
    LOG_ERROR("fseq_seek_frame %u", fseq->frame);
    return err;
  }

  return 0;
}

static int fseq_next(struct fseq *fseq)
{
  int err;
  
  fseq->frame++;

  if (fseq->frame < fseq_get_frame_count(fseq)) {
    fseq->tick += fseq_get_frame_ticks(fseq);


  } else if (fseq->mode & FSEQ_MODE_LOOP) {
    fseq->frame = 0;
    fseq->tick += fseq_get_frame_ticks(fseq);

    if ((err = fseq_seek_frame(fseq, fseq->frame))) {
      LOG_ERROR("fseq_seek_frame %u", fseq->frame);
      return err;
    }

  } else {
    fseq->tick = 0; // stop
  }

  return 0;
}

static int fseq_skip(struct fseq *fseq)
{
  int err;
  int ret = 0;

  if (!(fseq->mode & FSEQ_MODE_SKIP)) {
    return 0;
  }

  TickType_t tick = xTaskGetTickCount();

  while (fseq->tick && tick >= fseq->tick + fseq_get_frame_ticks(fseq)) {
    if ((err = fseq_next(fseq))) {
      return err;
    } else {
      ret++;
    }
  }

  if (ret > 0) {
    if ((err = fseq_seek_frame(fseq, fseq->frame))) {
      LOG_ERROR("fseq_seek_frame %u", fseq->frame);
      return err;
    }
  }

  return ret;
}

TickType_t fseq_tick(struct fseq *fseq)
{
  return fseq->tick;
}

bool fseq_ready(struct fseq *fseq)
{
  if (!fseq->tick) {
    return false;
  }

  return xTaskGetTickCount() >= fseq->tick;
}

int fseq_read(struct fseq *fseq, struct fseq_frame *frame)
{
  int err;
  int ret = 0;

  if ((ret = fseq_skip(fseq)) < 0) {
    return ret;
  }

  if ((err = fseq_read_frame(fseq, frame))) {
    return err;
  }

  if ((err = fseq_next(fseq))) {
    return err;
  }

  return ret;
}
