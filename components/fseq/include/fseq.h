#pragma once

#include <stdio.h>

#include <freertos/FreeRTOS.h>

/* bitmask of FSEQ_MODE_* bits */
enum fseq_mode {
  FSEQ_MODE_DEFAULT   = 0,

  // Loop back to first frame after playing last frame
  FSEQ_MODE_LOOP      = 1 << 0,

  // Skip frames instead of speeding up playback to catch up
  FSEQ_MODE_SKIP      = 1 << 1,
};

enum fseq_state {
  FSEQ_STATE_STOP,
  FSEQ_STATE_PLAY,
};

struct fseq;
struct fseq_frame {
  size_t size;
  uint8_t buf[];
};

/* Return size of fseq_frame in bytes (metadata + uint8_t buf).  */
static inline size_t fseq_frame_size(const struct fseq_frame *frame)
{
  return sizeof(*frame) + frame->size;
}

int fseq_new(struct fseq **fseqp, FILE *file);

/*
 * Allocate frame for use with fseq.
 */
int fseq_frame_new(struct fseq_frame **framep, struct fseq *fseq);

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
 *
 * Returns <0 on error, 0 on valid frame, >0 on skipped frames.
 */
int fseq_read(struct fseq *fseq, struct fseq_frame *frame);
