#include "leds_static.h"
#include "leds_task.h"
#include "leds_test.h"
#include "leds_artnet.h"

#include <logging.h>

static struct leds_color config_leds_color(struct config_color config_color, enum leds_parameter_type parameter_type)
{
  switch (parameter_type) {
    case LEDS_PARAMETER_NONE:
      return (struct leds_color) {
        .r = config_color.r * config_color.a / 255,
        .g = config_color.g * config_color.a / 255,
        .b = config_color.b * config_color.a / 255,
      };

    case LEDS_PARAMETER_DIMMER:
      return (struct leds_color) {
        .r = config_color.r,
        .g = config_color.g,
        .b = config_color.b,

        .dimmer = config_color.a,
      };

    case LEDS_PARAMETER_WHITE:
      return (struct leds_color) {
        .r = config_color.r,
        .g = config_color.g,
        .b = config_color.b,
      
        // TODO: fully transparent = only white, 50% = colors + 100% white?
        .white = (255 - config_color.a),
      };

    default:
      LOG_FATAL("parameter_type=%d", parameter_type);
  }
}

int config_leds_static(struct leds_state *state, const struct leds_config *config)
{
  state->static_.color = config_leds_color(config->static_color, leds_parameter_type_for_protocol(leds_protocol(state->leds)));

  LOG_INFO("color=%02x%02x%02x.%02x", state->static_.color.r, state->static_.color.g, state->static_.color.b, state->static_.color.parameter);

  // required to start initial tick
  notify_leds_task(state, 1 << LEDS_EVENT_STATIC_BIT);

  return 0;
}

int set_leds_static(struct leds_state *state, struct leds_color color)
{
  LOG_INFO("color=%02x%02x%02x.%02x", color.r, color.g, color.b, color.parameter);

  state->static_.color = color;

  notify_leds_task(state, 1 << LEDS_EVENT_STATIC_BIT);

  return 0;
}

TickType_t leds_static_wait(struct leds_state *state)
{
  return 0; // it's static, duh
}

bool leds_static_active(struct leds_state *state, EventBits_t bits)
{
  if (bits & (1 << LEDS_EVENT_STATIC_BIT)) {
    return true;
  }

  // TODO: active if artnet, test inactive?

  return false;
}

int leds_static_update(struct leds_state *state, EventBits_t bits)
{
  LOG_INFO("color=%02x%02x%02x.%02x", state->static_.color.r, state->static_.color.g, state->static_.color.b, state->static_.color.parameter);

  if (leds_set_all(state->leds, state->static_.color)) {
    LOG_ERROR("leds_set_all");
    return -1;
  }

  return 1;
}
