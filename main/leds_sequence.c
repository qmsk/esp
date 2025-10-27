#include "leds.h"
#include "leds_config.h"
#include "leds_state.h"
#include "leds_stats.h"
#include "leds_sequence.h"
#include "leds_task.h"
#include "tasks.h"
#include "user.h"

#include <logging.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

// config
struct leds_sequence_config leds_sequence_config = {

};

const struct config_file_path leds_sequence_paths[] = {
  { "/config/leds-sequence", "fseq" },
  { "/sdcard/leds-sequence", "fseq" },
  {}
};

const struct configtab leds_sequence_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &leds_sequence_config.enabled },
    .description = "Enable LEDs sequence input",
  },
  { CONFIG_TYPE_BOOL, "loop",
    .bool_type = { .value = &leds_sequence_config.loop },
    .description = "Loop LEDs sequence file",
  },
  { CONFIG_TYPE_FILE, "file",
    .file_type = { .value = leds_sequence_config.file, .size = sizeof(leds_sequence_config.file), .paths = leds_sequence_paths },
    .description = "Read LEDs sequence file from SD-Card",
  },
  {},
};

// state
struct leds_sequence {
  xTaskHandle task;

  struct fseq *fseq;
  struct fseq_frame *fseq_frame;
  enum fseq_mode fseq_mode;
} *leds_sequence;

int init_leds_sequence()
{
  const struct leds_sequence_config *config = &leds_sequence_config;
  FILE *file;
  int err;

  if (!config->enabled) {
    LOG_DEBUG("disabled");
    return 0;
  }

  if (!(leds_sequence = calloc(1, sizeof(*leds_sequence)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if (!(file = config_file_open(leds_sequence_paths, config->file))) {
    LOG_ERROR("config_file_open %s", config->file);
    return -1;
  }

  if ((err = fseq_new(&leds_sequence->fseq, file))) {
    LOG_ERROR("fseq_new");
    return err;
  }

  if ((err = fseq_frame_new(&leds_sequence->fseq_frame, leds_sequence->fseq))) {
    LOG_ERROR("fseq_frame_new");
    return err;
  }

  // stay in sync instead of slowing down playback
  leds_sequence->fseq_mode |= FSEQ_MODE_SKIP;

  if (config->loop) {
    leds_sequence->fseq_mode |= FSEQ_MODE_LOOP;
  }

  return 0;
}

int config_leds_sequence(struct leds_state *state, const struct leds_config *config)
{
  int err;

  if (!leds_sequence) {
    LOG_ERROR("leds-sequence not initialized");
    return -1;
  }

  if (!(state->sequence = calloc(1, sizeof(*state->sequence)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if (!(state->sequence->queue = xQueueCreate(1, fseq_frame_size(leds_sequence->fseq_frame)))) {
    LOG_ERROR("xQueueCreate: 1x%u", fseq_frame_size(leds_sequence->fseq_frame));
    return -1;
  }

  if ((err = fseq_frame_new(&state->sequence->fseq_frame, leds_sequence->fseq))) {
    LOG_ERROR("fseq_frame_new");
    return -1;
  }

  return 0;
}

static void leds_sequence_main(void *ctx)
{
  struct leds_sequence *leds_sequence = ctx;
  struct leds_sequence_stats *stats = &leds_sequence_stats;
  int err;

  if ((err = fseq_start(leds_sequence->fseq, leds_sequence->fseq_mode))) {
    LOG_ERROR("fseq_start");
    goto error;
  }

  for (;;) {
    // pre-read next into frame buffer
    WITH_STATS_TIMER(&stats->read) {
      if ((err = fseq_read(leds_sequence->fseq, leds_sequence->fseq_frame)) < 0) {
        user_alert(USER_ALERT_ERROR_LEDS_SEQUENCE_READ);
        LOG_ERROR("fseq_read");
        continue; // any way to recovery, or will this just be a busy-loop?
      } else if (err) {
        LOG_WARN("fseq_read: skip %d frames", err);

        stats_counter_add(&stats->skip, err);
      }
    }

    // wait for frame tick
    TickType_t tick = fseq_tick(leds_sequence->fseq);
    TickType_t t = xTaskGetTickCount();

    if (!tick) {
      LOG_INFO("stop");
      goto stop;
    } else if (tick > t) {
      vTaskDelay(tick - t);
    }

    // copy to each leds for output
    for (int i = 0; i < LEDS_COUNT; i++)
    {
      struct leds_state *state = &leds_states[i];

      if (!state->sequence) {
        continue;
      }

      xQueueOverwrite(state->sequence->queue, leds_sequence->fseq_frame);
      notify_leds_task(state, 1 << LEDS_EVENT_SEQUENCE_BIT);
    }
  }

error:
  user_alert(USER_ALERT_ERROR_LEDS_SEQUENCE);
  LOG_ERROR("task=%p stopped", leds_sequence->task);

stop:
  leds_sequence->task = NULL;
  vTaskDelete(NULL);
}

int start_leds_sequence()
{
  const struct leds_sequence_config *config = &leds_sequence_config;
  struct task_options task_options = {
    .main       = leds_sequence_main,
    .name       = LEDS_SEQUENCE_TASK_NAME,
    .stack_size = LEDS_SEQUENCE_TASK_STACK,
    .arg        = leds_sequence,
    .priority   = LEDS_TASK_PRIORITY,
    .handle     = &leds_sequence->task,
    .affinity   = LEDS_TASK_AFFINITY,
  };
  int err;

  if (!config->enabled) {
    LOG_DEBUG("disabled");
    return 0;
  }

  if (!leds_sequence) {
    LOG_ERROR("leds-sequence not initialized");
    return -1;
  }

  if (!leds_sequence->fseq) {
    LOG_ERROR("fseq not initialized");
    return -1;
  }

  if ((err = start_task(task_options))) {
    LOG_ERROR("start_task");
    return err;
  } else {
    LOG_INFO("start task=%p", leds_sequence->task);
  }

  return 0;
}

static int set_leds_sequence(struct leds_state *state, const struct fseq_frame *frame)
{
  const struct leds_config *config = state->config;
  struct leds_format_params params = {
    .count = config->sequence_leds_count,
    .segment = config->sequence_leds_segment,
    .group = config->sequence_leds_group,
    .offset = config->sequence_leds_offset,
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

TickType_t leds_sequence_wait(struct leds_state *state)
{
  return 0;
}

bool leds_sequence_active(struct leds_state *state, EventBits_t event_bits)
{
  if (!state->sequence) {
    return false;
  }

  if (event_bits & (1 << LEDS_EVENT_SEQUENCE_BIT)) {
    return true;
  }

  return false;
}

int leds_sequence_update(struct leds_state *state, EventBits_t event_bits)
{
  int err;

  if (!xQueueReceive(state->sequence->queue, state->sequence->fseq_frame, 0)) {
    LOG_WARN("xQueueReceive: queue empty");
    return 0;
  }

  if ((err = set_leds_sequence(state, state->sequence->fseq_frame))) {
    LOG_ERROR("set_leds_sequence");
    return err;
  }

  return 1; // updated
}
