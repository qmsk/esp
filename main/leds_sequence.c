#include "leds.h"
#include "leds_config.h"
#include "leds_state.h"
#include "leds_sequence.h"
#include "leds_task.h"

#include <logging.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

static int set_leds_sequence(struct leds_state *state, const struct fseq_frame *frame)
{
  const struct leds_config *config = state->config;
  struct leds_format_params params = {
    .count = config->sequence_leds_count,
    .offset = config->sequence_leds_offset,
    .segment = config->sequence_leds_segment,
  };
  const uint8_t *ptr = frame->buf;
  size_t len = frame->size;
  int err;

  if (config->sequence_channel_start) {
    unsigned offset = config->sequence_channel_start - 1;

    if (offset > len) {
      ptr = NULL;
      len = 0;
    } else {
      ptr += offset;
      len -= offset;
    }
  }

  if (config->sequence_channel_count) {
    if (config->sequence_channel_count > len) {

    } else {
      len = config->sequence_channel_count;
    }
  }

  if ((err = leds_set_format(state->leds, config->sequence_format, ptr, len, params))) {
    LOG_WARN("leds_set_format");
    return err;
  }

  return 0;
}

int init_leds_sequence(struct leds_state *state, const struct leds_config *config)
{
  FILE *file;
  int err;

  if (!(state->sequence = calloc(1, sizeof(*state->sequence)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if (!(file = config_file_open(leds_sequence_paths, config->sequence_file))) {
    LOG_ERROR("config_file_open %s", config->sequence_file);
    return -1;
  }

  if ((err = fseq_new(&state->sequence->fseq, file))) {
    LOG_ERROR("fseq_new");
    return err;
  }

  if ((err = fseq_frame_init(&state->sequence->frame, state->sequence->fseq))) {
    LOG_ERROR("fseq_frame_init");
    return err;
  }

  return 0;
}

int start_leds_sequence(struct leds_state *state, const struct leds_config *config)
{
  enum fseq_mode mode = FSEQ_MODE_DEFAULT;
  int err;

  if (!state->sequence->fseq) {
    LOG_WARN("fseq not initialized");
    return 0;
  }

  if (config->sequence_loop) {
    mode |= FSEQ_MODE_LOOP;
  }

  if ((err = fseq_start(state->sequence->fseq, mode))) {
    LOG_ERROR("fseq_start");
    return err;
  }

  notify_leds_task(state, 1 << LEDS_EVENT_SEQUENCE_BIT);

  return 0;
}

TickType_t leds_sequence_wait(struct leds_state *state)
{
  if (!state->sequence->fseq) {
    return 0;
  }

  return fseq_tick(state->sequence->fseq);
}

bool leds_sequence_active(struct leds_state *state, EventBits_t event_bits)
{
  if (!state->sequence->fseq) {
    return false;
  }

  if (fseq_ready(state->sequence->fseq)) {
    return true;
  }

  return false;
}

int leds_sequence_update(struct leds_state *state, EventBits_t event_bits)
{
  int err;

  // TODO: skip frames?
  if ((err = fseq_read(state->sequence->fseq, &state->sequence->frame))) {
    LOG_ERROR("fseq_read");
    return err;
  }

  if ((err = set_leds_sequence(state, &state->sequence->frame))) {
    LOG_ERROR("set_leds_sequence");
    return err;
  }

  return 1; // updated
}
