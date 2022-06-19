#include "leds_test.h"
#include "artnet_state.h"

#include <logging.h>

struct leds_test_state leds_test_state;

void trigger_leds_test()
{
  if (leds_test_state.mode++ >= TEST_MODE_END) {
    leds_test_state.mode = TEST_MODE_CHASE;
  }

  LOG_INFO("mode=%d auto=%d", leds_test_state.mode, leds_test_state.auto_mode);

  // start or continue test
  notify_artnet_outputs(1 << LEDS_ARTNET_EVENT_TEST_BIT);
}

void auto_leds_test()
{
  leds_test_state.auto_mode = true;

  LOG_INFO("mode=%d auto=%d", leds_test_state.mode, leds_test_state.auto_mode);

  // start or continue test
  notify_artnet_outputs(1 << LEDS_ARTNET_EVENT_TEST_BIT);
}

void reset_leds_test()
{
  leds_test_state.mode = 0;
  leds_test_state.auto_mode = false;

  LOG_INFO("mode=%d auto=%d", leds_test_state.mode, leds_test_state.auto_mode);

  // force-sync will abort the test mode
  notify_artnet_outputs(1 << LEDS_ARTNET_EVENT_SYNC_BIT);
}
