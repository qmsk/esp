#include <user_leds.h>
#include "user_leds.h"

#include <logging.h>

#include <driver/gpio.h>
#include <esp_err.h>

// 100ms on / 1800ms off
#define USER_LEDS_SLOW_PERIOD_ON 100
#define USER_LEDS_SLOW_PERIOD_OFF 1800

// 100ms on / 100ms off
#define USER_LEDS_FAST_PERIOD 100

// 10ms on
#define USER_LEDS_FLASH_PERIOD 10

// 50ms off / 200ms on
#define USER_LEDS_PULSE_PERIOD_OFF 50
#define USER_LEDS_PULSE_PERIOD_ON 200

// 10ms read wait
#define USER_LEDS_READ_WAIT_PERIOD 10
#define USER_LEDS_READ_NOTIFY_BIT 0x1

static inline void user_leds_output_mode(struct user_leds *leds)
{
  LOG_DEBUG("gpio=%d", leds->options.gpio);

  gpio_set_direction(leds->options.gpio, GPIO_MODE_OUTPUT);
}

static inline void user_leds_off(struct user_leds *leds)
{
  gpio_set_level(leds->options.gpio, leds->options.inverted ? 1 : 0);
}

static inline void user_leds_on(struct user_leds *leds)
{
  gpio_set_level(leds->options.gpio, leds->options.inverted ? 0 : 1);
}

static inline void user_leds_input_mode(struct user_leds *leds)
{
  LOG_DEBUG("gpio=%d", leds->options.gpio);

  gpio_set_direction(leds->options.gpio, GPIO_MODE_INPUT);
}

static inline int user_leds_input_read(struct user_leds *leds)
{
  int level = gpio_get_level(leds->options.gpio);

  LOG_DEBUG("gpio=%d: level=%d", leds->options.gpio, level);

  return leds->options.inverted ? !level : level;
}

static void user_leds_set_mode(struct user_leds *leds, enum user_leds_mode mode)
{
  user_leds_output_mode(leds);

  LOG_DEBUG("gpio=%d: mode=%d", leds->options.gpio, mode);

  switch(leds->mode = mode) {
    case USER_LEDS_OFF:
      leds->state = 0;
      user_leds_off(leds);
      break;

    case USER_LEDS_ON:
      leds->state = 1;
      user_leds_on(leds);
      break;

    case USER_LEDS_SLOW:
    case USER_LEDS_FAST:
    case USER_LEDS_FLASH:
    case USER_LEDS_PULSE:
      leds->state = 0;
      user_leds_on(leds);
      break;
  }
}

static void user_leds_read_mode(struct user_leds *leds)
{
  LOG_DEBUG("gpio=%d", leds->options.gpio);

  user_leds_off(leds);
  user_leds_input_mode(leds);

  leds->input = true;
  leds->input_wait = true;
}

static void user_leds_read_notify(struct user_leds *leds, int level)
{
  LOG_DEBUG("gpio=%d: read_task=%p level=%d", leds->options.gpio, leds->read_task, level);

  if (leds->read_task) {
    xTaskNotify(leds->read_task, level ? USER_LEDS_READ_NOTIFY_BIT : 0, eSetBits);

    leds->read_task = false;
  }
}

static portTickType user_leds_tick(struct user_leds *leds)
{
  if (leds->input_wait) {
   // wait for input to settle
    LOG_DEBUG("gpio=%d: input wait", leds->options.gpio);

    leds->input_wait = 0;

    return USER_LEDS_READ_WAIT_PERIOD / portTICK_RATE_MS;
  }

  if (leds->input) {
    // read input and notify waiting task
    user_leds_read_notify(leds, user_leds_input_read(leds));

    // revert back to output mode
    user_leds_output_mode(leds);

    leds->input = false;

    // fall through
  }

  switch(leds->mode) {
    case USER_LEDS_OFF:
      leds->state = 0;
      user_leds_off(leds);
      return 0;

    case USER_LEDS_ON:
      leds->state = 1;
      user_leds_on(leds);
      return 0;

    case USER_LEDS_SLOW:
      if (leds->state) {
        leds->state = 0;
        user_leds_off(leds);
        return USER_LEDS_SLOW_PERIOD_OFF / portTICK_RATE_MS;
      } else {
        leds->state = 1;
        user_leds_on(leds);
        return USER_LEDS_SLOW_PERIOD_ON / portTICK_RATE_MS;
      }

    case USER_LEDS_FAST:
      if (leds->state) {
        leds->state = 0;
        user_leds_off(leds);
      } else {
        leds->state = 1;
        user_leds_on(leds);
      }
      return USER_LEDS_FAST_PERIOD / portTICK_RATE_MS;

    case USER_LEDS_FLASH:
      if (leds->state) {
        user_leds_off(leds);
        return 0;
      } else {
        leds->state = 1;
        return USER_LEDS_FLASH_PERIOD / portTICK_RATE_MS;
      }

    case USER_LEDS_PULSE:
      if (leds->state) {
        leds->state = 0;
        user_leds_off(leds);
        return USER_LEDS_PULSE_PERIOD_OFF / portTICK_RATE_MS;
      } else {
        leds->state = 1;
        user_leds_on(leds);
        return USER_LEDS_PULSE_PERIOD_ON / portTICK_RATE_MS;
      }

    default:
      return 0;
  }
}

// schedule next tick
static TickType_t user_leds_schedule(struct user_leds *leds, TickType_t period)
{
  TickType_t tick = xTaskGetTickCount();

  if (period == 0) {
    // reset phase
    leds->tick = 0;

    // indefinite period
    return portMAX_DELAY;

  } else if (leds->tick == 0) {
    // start phase
    leds->tick = tick + period;

    return period;

  } else if (leds->tick + period > tick) {
    leds->tick += period;

    return leds->tick - tick;

  } else {
    leds->tick += period;

    // missed tick, catchup
    return 0;
  }
}

void user_leds_main(void *arg)
{
  struct user_leds *leds = arg;
  struct user_leds_event event;

  leds->tick = 0;

  for (;;) {
    TickType_t period = user_leds_tick(leds);
    TickType_t delay = user_leds_schedule(leds, period);

    LOG_DEBUG("input=%d mode=%d state=%d -> period=%u delay=%u", leds->input, leds->mode, leds->state, period, delay);

    if (!xQueueReceive(leds->queue, &event, delay)) {
      // tick next frame on timeout
      continue;
    }

    switch (event.op) {
      case USER_LEDS_SET:
        user_leds_set_mode(leds, event.mode);
        break;

      case USER_LEDS_READ:
        user_leds_read_mode(leds);
        break;
    }
  }
}

int user_leds_init_gpio(struct user_leds *leds, const struct user_leds_options options)
{
  gpio_config_t config = {
      .pin_bit_mask = (1 << options.gpio),
      .mode         = GPIO_MODE_OUTPUT,
      .pull_up_en   = options.inverted ? 1 : 0,
      .pull_down_en = options.inverted ? 0 : 1,
  };
  esp_err_t err;

  LOG_DEBUG("gpio=%d inverted=%d", options.gpio, options.inverted);

  if ((err = gpio_config(&config))) {
    LOG_ERROR("gpio_config: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int user_leds_new(struct user_leds **ledp, const struct user_leds_options options, enum user_leds_mode mode)
{
  struct user_leds *leds;
  int err;

  if (!(leds = calloc(1, sizeof(*leds)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  leds->options = options;
  leds->set_mode = mode;

  // set initial state
  user_leds_set_mode(leds, mode);

  // enable GPIo output
  if ((err = user_leds_init_gpio(leds, options))) {
    LOG_ERROR("user_leds_init_gpio");
    goto error;
  }

  // setup mutex
  if (!(leds->mutex = xSemaphoreCreateMutex())) {
    LOG_ERROR("xSemaphoreCreateMutex");
    err = -1;
    goto error;
  }

  // setup task
  if ((leds->queue = xQueueCreate(1, sizeof(struct user_leds_event))) == NULL) {
    LOG_ERROR("xQueueCreate");
    err = -1;
    goto error;
  }

  *ledp = leds;

  return 0;

error:
  free(leds);

  return err;
}

int user_leds_set(struct user_leds *leds, enum user_leds_mode mode, TickType_t timeout)
{
  struct user_leds_event event = { USER_LEDS_SET, mode };
  int err = 0;

  if (!xSemaphoreTake(leds->mutex, timeout)) {
    LOG_DEBUG("xSemaphoreTake: timeout");
    return 1;
  }

  LOG_DEBUG("gpio=%d: mode=%d", leds->options.gpio, mode);

  leds->set_mode = mode;

  if (leds->override) {
    // skip set op
    LOG_DEBUG("override");

  } else if (xQueueOverwrite(leds->queue, &event) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    err = -1;
    goto error;
  }

error:
  if (!xSemaphoreGive(leds->mutex)) {
    LOG_WARN("xSemaphoreGive");
  }

  return err;
}

int user_ledss_override(struct user_leds *leds, enum user_leds_mode mode)
{
  struct user_leds_event event = { USER_LEDS_SET, mode };
  int err = 0;

  if (!xSemaphoreTake(leds->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  LOG_DEBUG("gpio=%d: mode=%d", leds->options.gpio, mode);

  leds->override = true;

  if (xQueueOverwrite(leds->queue, &event) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    err = -1;
    goto error;
  }

error:
  if (!xSemaphoreGive(leds->mutex)) {
    LOG_WARN("xSemaphoreGive");
  }

  return err;
}

int user_ledss_revert(struct user_leds *leds)
{
  struct user_leds_event event = { USER_LEDS_SET, 0 };
  int err = 0;

  if (!xSemaphoreTake(leds->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  LOG_DEBUG("gpio=%d: set_mode=%d", leds->options.gpio, leds->set_mode);

  leds->override = false;

  // use last mode set
  event.mode = leds->set_mode;

  if (xQueueOverwrite(leds->queue, &event) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    err = -1;
    goto error;
  }

error:
  if (!xSemaphoreGive(leds->mutex)) {
    LOG_WARN("xSemaphoreGive");
  }

  return err;
}

int user_leds_read(struct user_leds *leds)
{
  struct user_leds_event event = { USER_LEDS_READ, 0 };
  uint32_t notify_value;
  int ret = 0;

  if (!xSemaphoreTake(leds->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  // request notify
  LOG_DEBUG("gpio=%d: read_task=%p", leds->options.gpio, xTaskGetCurrentTaskHandle());

  leds->read_task = xTaskGetCurrentTaskHandle();

  // set input mode and read
  if (xQueueOverwrite(leds->queue, &event) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    ret = -1;
    goto error;
  }

  // wait
  if (!xTaskNotifyWait(0, USER_LEDS_READ_NOTIFY_BIT, &notify_value, portMAX_DELAY)) {
    LOG_ERROR("xTaskNotifyWait");
    ret = -1;
    goto error;
  }

  LOG_DEBUG("gpio=%d: notify_value=%04x", leds->options.gpio, notify_value);

  if (notify_value & USER_LEDS_READ_NOTIFY_BIT) {
    ret = 1;
  }

error:
  if (!xSemaphoreGive(leds->mutex)) {
    LOG_WARN("xSemaphoreGive");
  }

  return ret;
}
