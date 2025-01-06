  #include "user.h"
#include "user_leds_state.h"
#include "user_log.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>
#include <json.h>

#define TICK_MS(current_tick, tick) (tick ? (current_tick - tick) * portTICK_RATE_MS : 0)

static int user_api_write_status(struct json_writer *w, void *ctx)
{
  const struct user_log *state_log = &user_state_log;
  const struct user_log *activity_log = &user_activity_log;
  const struct user_log *alert_log = &user_alert_log;
  TickType_t tick = xTaskGetTickCount();

  return JSON_WRITE_OBJECT(w, 
        JSON_WRITE_MEMBER_OBJECT(w, "state", 
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
