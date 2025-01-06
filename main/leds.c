#include "leds.h"
#include "leds_artnet.h"
#include "leds_state.h"
#include "leds_config.h"
#include "leds_stats.h"
#include "leds_sequence.h"
#include "leds_task.h"
#include "leds_test.h"
#include "atx_psu_state.h"
#include "user.h"

#include <leds.h>
#include <leds_stats.h>

#include <logging.h>

// wait up to one second for ATX-PSU to signal power good
#define LEDS_ATX_PSU_POWER_GOOD_TIMEOUT (1000 / portTICK_PERIOD_MS)

struct leds_state leds_states[LEDS_COUNT] = {};

int init_leds()
{
  int err;

  init_leds_stats();
  leds_reset_interface_stats();

#if CONFIG_LEDS_GPIO_ENABLED
  if ((err = init_leds_gpio())) {
    LOG_ERROR("init_leds_gpio");
    return 0;
  }
#endif

#if CONFIG_LEDS_SPI_ENABLED
  if ((err = init_leds_spi())) {
    LOG_ERROR("init_leds_spi");
    return 0;
  }
#endif

#if CONFIG_LEDS_UART_ENABLED
  if ((err = init_leds_uart())) {
    LOG_ERROR("init_leds_uart");
    return 0;
  }
#endif

#if CONFIG_LEDS_I2S_ENABLED
  if ((err = init_leds_i2s())) {
    LOG_ERROR("init_leds_i2s");
    return 0;
  }
#endif

  if ((err = init_leds_sequence())) {
    LOG_ERROR("init_leds_sequence");
    return 0;
  }

  for (int i = 0; i < LEDS_COUNT; i++)
  {
    struct leds_state *state = &leds_states[i];
    const struct leds_config *config = &leds_configs[i];

    state->index = i;
    state->config = config;

    if (!config->enabled) {
      continue;
    }

    if ((err = config_leds(state, config))) {
      LOG_ERROR("leds%d: config_leds", i+1);
      return err;
    }

    if ((err = init_leds_task(state, config))) {
      LOG_ERROR("leds%d: init_leds_task", i+1);
      return err;
    }

    if (config->test_enabled) {
      if ((err = init_leds_test(state, config))) {
        LOG_ERROR("leds%d: init_leds_test", i + 1);
        return err;
      }
    }

    if (config->artnet_enabled) {
      if ((err = init_leds_artnet(state, i, config))) {
        LOG_ERROR("leds%d: init_leds_artnet", i + 1);
        return err;
      }
    }

    if (config->sequence_enabled) {
      if ((err = config_leds_sequence(state, config))) {
        LOG_ERROR("leds%d: config_leds_sequence", i + 1);
        return err;
      }
    }
  }

  return 0;
}

int start_leds()
{
  int err;

  for (int i = 0; i < LEDS_COUNT; i++)
  {
    struct leds_state *state = &leds_states[i];
    const struct leds_config *config = &leds_configs[i];

    if (!config->enabled) {
      continue;
    }

    if (!state->leds) {
      LOG_WARN("leds%d: not initialized", i + 1);
      continue;
    }

    if ((err = start_leds_task(state, config))) {
      LOG_ERROR("leds%d: start_leds_task", i + 1);
      return err;
    }

    if (config->artnet_enabled) {
      if ((err = start_leds_artnet(state, config))) {
        LOG_ERROR("leds%d: start_leds_artnet", i + 1);
        return err;
      }
    }
  }

  if ((err = start_leds_sequence())) {
    LOG_ERROR("start_leds_sequence");
    return err;
  }

  return 0;
}

int check_leds_interface(struct leds_state *state)
{
  if (!state->leds) {
    LOG_WARN("leds%d: not initialized", state->index + 1);
    return -1;
  }

  switch (leds_interface(state->leds)) {
    case LEDS_INTERFACE_I2S:
      return check_leds_i2s(state);

    default:
      return 0;
  }
}

void force_leds_active(struct leds_state *state)
{
  // wait for power_good before sending first frame
  wait_atx_psu_bit(ATX_PSU_BIT_LEDS1 + state->index, LEDS_ATX_PSU_POWER_GOOD_TIMEOUT);
}

void update_leds_active(struct leds_state *state)
{
  if (leds_is_active(state->leds)) {
    // wait for power_good before sending first frame
    wait_atx_psu_bit(ATX_PSU_BIT_LEDS1 + state->index, LEDS_ATX_PSU_POWER_GOOD_TIMEOUT);
  } else {
    clear_atx_psu_bit(ATX_PSU_BIT_LEDS1 + state->index);
  }
}

void clear_leds_active(struct leds_state *state)
{
  clear_atx_psu_bit(ATX_PSU_BIT_LEDS1 + state->index);
}

int update_leds(struct leds_state *state, enum user_activity leds_activity)
{
  int err;

  update_leds_active(state);

  if ((err = check_leds_interface(state))) {
    return err;
  }

  if (leds_activity) {
    user_activity(leds_activity);
  }

  if ((err = leds_tx(state->leds))) {
    LOG_ERROR("leds_tx");
    return err;
  }

  return 0;
}

int clear_leds(struct leds_state *state)
{
  int err;

  if ((err = check_leds_interface(state))) {
    return err;
  }

  if ((err = leds_clear_all(state->leds))) {
    LOG_ERROR("leds_set_all");
    return err;
  }

  if ((err = leds_tx(state->leds))) {
    LOG_ERROR("leds_tx");
    return err;
  }

  clear_leds_active(state);

  return 0;
}

int test_leds_mode(struct leds_state *state, enum leds_test_mode mode)
{
  int err = 0;

  LOG_INFO("mode=%d", mode);

  if ((err = check_leds_interface(state))) {
    return err;
  }

  force_leds_active(state);
  user_activity(USER_ACTIVITY_LEDS);

  // animate
  TickType_t tick = xTaskGetTickCount();
  int ticks;

  for (unsigned frame = 0; ; frame++) {
    if ((ticks = leds_set_test(state->leds, mode, frame)) < 0) {
      LOG_ERROR("leds%d: leds_set_test(%d, %u)", state->index + 1, mode, frame);
      goto error;
    }

    if ((err = leds_tx(state->leds))) {
      LOG_ERROR("leds%d: leds_tx", state->index + 1);
      goto error;
    }

    if (ticks) {
      vTaskDelayUntil(&tick, ticks);
    } else {
      break;
    }
  }

error:
  update_leds_active(state);

  return err;
}

int test_leds(struct leds_state *state)
{
  int err;

  for (enum leds_test_mode mode = 0; mode < TEST_MODE_COUNT; mode++) {
    if ((err = test_leds_mode(state, mode))) {
      LOG_ERROR("leds%d: test_leds", state->index + 1);
      return err;
    }
  }

  return 0;
}
