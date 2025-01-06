  #include "user.h"
#include "user_leds_state.h"
#include "user_log.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>
#include <json.h>

#include <string.h>

#define TICK_MS(current_tick, tick) (tick ? (current_tick - tick) * portTICK_RATE_MS : 0)

static int user_api_write_status(struct json_writer *w, void *ctx)
{
  const struct user_log *power_log = &user_power_log;
  const struct user_log *state_log = &user_state_log;
  const struct user_log *activity_log = &user_activity_log;
  const struct user_log *alert_log = &user_alert_log;
  TickType_t tick = xTaskGetTickCount();

  return JSON_WRITE_OBJECT(w, 
        JSON_WRITE_MEMBER_UINT(w, "tick", tick)
    ||  JSON_WRITE_MEMBER_UINT(w, "tick_rate_ms", portTICK_RATE_MS)
    ||  JSON_WRITE_MEMBER_OBJECT(w, "power",
              JSON_WRITE_MEMBER_UINT(w, "tick", power_log->tick)
          ||  JSON_WRITE_MEMBER_UINT(w, "tick_ms", TICK_MS(tick, power_log->tick))
          ||  JSON_WRITE_MEMBER_STRING(w, "type", user_power_str(power_log->state))
          ||  JSON_WRITE_MEMBER_STRING(w, "leds_state", config_enum_to_string(user_leds_state_enum, USER_LEDS_ON)) // hardcoded
        )
    ||  JSON_WRITE_MEMBER_OBJECT(w, "state",
              JSON_WRITE_MEMBER_UINT(w, "tick", state_log->tick)
          ||  JSON_WRITE_MEMBER_UINT(w, "tick_ms", TICK_MS(tick, state_log->tick))
          ||  JSON_WRITE_MEMBER_STRING(w, "type", user_state_str(state_log->state))
        #if CONFIG_STATUS_LEDS_USER_ENABLED
          ||  JSON_WRITE_MEMBER_STRING(w, "leds_state", config_enum_to_string(user_leds_state_enum, user_leds_state[USER_LED]))
        #endif
        )
    ||  JSON_WRITE_MEMBER_OBJECT(w, "activity",
              JSON_WRITE_MEMBER_UINT(w, "tick", activity_log->tick)
          ||  JSON_WRITE_MEMBER_UINT(w, "tick_ms", TICK_MS(tick, activity_log->tick))
          ||  JSON_WRITE_MEMBER_STRING(w, "type", user_activity_str(activity_log->state))
        #if CONFIG_STATUS_LEDS_FLASH_ENABLED
          ||  JSON_WRITE_MEMBER_STRING(w, "leds_state", config_enum_to_string(user_leds_state_enum, user_leds_state[FLASH_LED]))
        #endif
        )
    ||  JSON_WRITE_MEMBER_OBJECT(w, "alert",
              JSON_WRITE_MEMBER_UINT(w, "tick", alert_log->tick)
          ||  JSON_WRITE_MEMBER_UINT(w, "tick_ms", TICK_MS(tick, alert_log->tick))
          ||  JSON_WRITE_MEMBER_STRING(w, "type", user_alert_str(alert_log->state))
        #if CONFIG_STATUS_LEDS_ALERT_ENABLED
          ||  JSON_WRITE_MEMBER_STRING(w, "leds_state", config_enum_to_string(user_leds_state_enum, user_leds_state[ALERT_LED]))
        #endif
        )
  );
}

int user_api_get_status(struct http_request *request, struct http_response *response, void *ctx)
{
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = write_http_response_json(response, user_api_write_status, NULL))) {
    LOG_WARN("write_http_response_json -> user_api_write_status");
    return err;
  }

  return 0;
}

struct user_api_button {
  bool config, test;

  enum user_leds_input_event config_event;
  enum user_leds_input_event test_event;
};

int user_api_event_parse(const char *value, enum user_leds_input_event *event)
{
  if (strcmp(value, "press") == 0) {
    *event = USER_LEDS_INPUT_PRESS;
    return 0;
  } else if (strcmp(value, "hold") == 0) {
    *event = USER_LEDS_INPUT_HOLD;
    return 0;
  } else if (strcmp(value, "release") == 0) {
    *event = USER_LEDS_INPUT_RELEASE;
    return 0;
  } else {
    return 1;
  }
}

int user_api_button_parse_form(struct http_request *request, struct user_api_button *api)
{
  char *key, *value;
  int err;

  while (!(err = http_request_form(request, &key, &value))) {
    if (strcmp(key, "config") == 0) {
      if (user_api_event_parse(value, &api->config_event)) {
        LOG_WARN("user_api_event_parse: key=%s value=%s", key, value);
        return HTTP_UNPROCESSABLE_ENTITY;
      } else {
        api->config = true;
      }
    } else if (strcmp(key, "test") == 0) {
      if (user_api_event_parse(value, &api->test_event)) {
        LOG_WARN("user_api_event_parse: key=%s value=%s", key, value);
        return HTTP_UNPROCESSABLE_ENTITY;
      } else {
        api->test = true;
      }
    }
  }

  if (err < 0) {
    LOG_ERROR("http_request_form");
    return err;
  }

  return 0;
}

int user_api_button(const struct user_api_button *api)
{
  if (api->config) {
    switch (api->config_event) {
      case USER_LEDS_INPUT_PRESS:
        LOG_INFO("config mode");
        user_config_mode();
        break;

      case USER_LEDS_INPUT_HOLD:
        LOG_INFO("config reset");
        user_config_reset();
        break;

      case USER_LEDS_INPUT_RELEASE:
        break;

      default:
        LOG_ERROR("unknown event");
        break;
    }
  }

  if (api->test) {
    switch (api->test_event) {
      case USER_LEDS_INPUT_PRESS:
        LOG_INFO("test trigger");
        user_test_trigger();
        break;

      case USER_LEDS_INPUT_HOLD:
        LOG_INFO("test hold");
        user_test_hold();
        break;

      case USER_LEDS_INPUT_RELEASE:
        LOG_INFO("test release");
        user_test_cancel();
        break;

      default:
        LOG_ERROR("unknown event");
        break;
    }
  }

  return HTTP_NO_CONTENT;
}

int user_api_post_button(struct http_request *request, struct http_response *response, void *ctx)
{
  struct user_api_button api = {};
  const struct http_request_headers *headers;
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  switch (headers->content_type) {
    case HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED:
      if ((err = user_api_button_parse_form(request, &api))) {
        return err;
      }

      break;

    default:
      LOG_WARN("Unknown Content-Type");

      return HTTP_UNSUPPORTED_MEDIA_TYPE;
  }

  if ((err = user_api_button(&api)) < 0) {
    LOG_WARN("user_api_button");
    return err;
  }

  return err;
}
