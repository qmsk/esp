  #include "user.h"
#include "user_leds_state.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>
#include <json.h>

static int user_api_write_status(struct json_writer *w, void *ctx)
{
  return JSON_WRITE_OBJECT(w, 
        JSON_WRITE_MEMBER_OBJECT(w, "state", 
        #if CONFIG_STATUS_LEDS_USER_ENABLED
          JSON_WRITE_MEMBER_STRING(w, "leds_state", config_enum_to_string(user_leds_state_enum, user_leds_state[USER_LED]))
        #endif
        )
    ||  JSON_WRITE_MEMBER_OBJECT(w, "activity", 
        #if CONFIG_STATUS_LEDS_FLASH_ENABLED
          JSON_WRITE_MEMBER_STRING(w, "leds_state", config_enum_to_string(user_leds_state_enum, user_leds_state[FLASH_LED]))
        #endif
        )
    ||  JSON_WRITE_MEMBER_OBJECT(w, "alert", 
        #if CONFIG_STATUS_LEDS_ALERT_ENABLED
          JSON_WRITE_MEMBER_STRING(w, "leds_state", config_enum_to_string(user_leds_state_enum, user_leds_state[ALERT_LED]))
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
