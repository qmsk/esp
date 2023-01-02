#pragma once

#include <stdio.h>

#include <freertos/FreeRTOS.h>

enum fseq_mode {
  FSEQ_MODE_DEFAULT   = 0,

  FSEQ_MODE_LOOP      = 1,
};

enum fseq_state {
  FSEQ_STATE_STOP,
  FSEQ_STATE_PLAY,
};

struct fseq;
struct fseq_frame {
  uint8_t *buf;
  size_t size;
};

int fseq_new(struct fseq **fseqp, FILE *file);

/*
 * Initialize frame for use with fseq.
 */
int fseq_frame_init(struct fseq_frame *frame, struct fseq *fseq);

/*
 * Return size of fseq frame in bytes (uint8_t channels).
 */
size_t fseq_frame_size(struct fseq *fseq);

/*
 * Return current fseq state.
 */
enum fseq_state fseq_state(struct fseq *fseq);

/*
 * Start fseq playback from first frame at current tick.
 */
int fseq_start(struct fseq *fseq, enum fseq_mode mode);

/*
 * Return tick for next fseq frame, 0 if stopped.
 */
TickType_t fseq_tick(struct fseq *fseq);

/*
 * Return true if next frame is ready for playout.
 */
bool fseq_ready(struct fseq *fseq);

/*
 * Return next frame data in *frame.
 */
int fseq_read(struct fseq *fseq, struct fseq_frame *frame);
