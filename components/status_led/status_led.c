#include <status_led.h>
#include "status_led.h"

#include <logging.h>

#include <driver/gpio.h>
#include <esp_err.h>

// 100ms on / 1800ms off
#define STATUS_LED_SLOW_PERIOD_ON 100
#define STATUS_LED_SLOW_PERIOD_OFF 1800

// 100ms on / 100ms off
#define STATUS_LED_FAST_PERIOD 100

// 10ms on
#define STATUS_LED_FLASH_PERIOD 10

// 50ms off / 200ms on
#define STATUS_LED_PULSE_PERIOD_OFF 50
#define STATUS_LED_PULSE_PERIOD_ON 200

// 10ms read wait
#define STATUS_LED_READ_WAIT_PERIOD 10
#define STATUS_LED_READ_NOTIFY_BIT 0x1

#if CONFIG_IDF_TARGET_ESP8266
# define STATUS_LED_TASK_STACK 512
#elif CONFIG_IDF_TARGET_ESP32
# define STATUS_LED_TASK_STACK 1024
#endif

#define STATUS_LED_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

static inline void status_led_output_mode(struct status_led *led)
{
  LOG_DEBUG("gpio=%d", led->options.gpio);

  gpio_set_direction(led->options.gpio, GPIO_MODE_OUTPUT);
}

static inline void status_led_off(struct status_led *led)
{
  gpio_set_level(led->options.gpio, led->options.inverted ? 1 : 0);
}

static inline void status_led_on(struct status_led *led)
{
  gpio_set_level(led->options.gpio, led->options.inverted ? 0 : 1);
}

static inline void status_led_input_mode(struct status_led *led)
{
  LOG_DEBUG("gpio=%d", led->options.gpio);

  gpio_set_direction(led->options.gpio, GPIO_MODE_INPUT);
}

static inline int status_led_input_read(struct status_led *led)
{
  int level = gpio_get_level(led->options.gpio);

  LOG_DEBUG("gpio=%d: level=%d", led->options.gpio, level);

  return led->options.inverted ? !level : level;
}

static void status_led_set_mode(struct status_led *led, enum status_led_mode mode)
{
  status_led_output_mode(led);

  LOG_DEBUG("gpio=%d: mode=%d", led->options.gpio, mode);

  switch(led->mode = mode) {
    case STATUS_LED_OFF:
      led->state = 0;
      status_led_off(led);
      break;

    case STATUS_LED_ON:
      led->state = 1;
      status_led_on(led);
      break;

    case STATUS_LED_SLOW:
    case STATUS_LED_FAST:
    case STATUS_LED_FLASH:
    case STATUS_LED_PULSE:
      led->state = 0;
      status_led_on(led);
      break;
  }
}

static void status_led_read_mode(struct status_led *led)
{
  LOG_DEBUG("gpio=%d", led->options.gpio);

  status_led_off(led);
  status_led_input_mode(led);

  led->input = true;
  led->input_wait = true;
}

static void status_led_read_notify(struct status_led *led, int level)
{
  LOG_DEBUG("gpio=%d: read_task=%p level=%d", led->options.gpio, led->read_task, level);

  if (led->read_task) {
    xTaskNotify(led->read_task, level ? STATUS_LED_READ_NOTIFY_BIT : 0, eSetBits);

    led->read_task = false;
  }
}

static portTickType status_led_tick(struct status_led *led)
{
  if (led->input_wait) {
   // wait for input to settle
    LOG_DEBUG("gpio=%d: input wait", led->options.gpio);

    led->input_wait = 0;

    return STATUS_LED_READ_WAIT_PERIOD / portTICK_RATE_MS;
  }

  if (led->input) {
    // read input and notify waiting task
    status_led_read_notify(led, status_led_input_read(led));

    // revert back to output mode
    status_led_output_mode(led);

    led->input = false;

    // fall through
  }

  switch(led->mode) {
    case STATUS_LED_OFF:
      led->state = 0;
      status_led_off(led);
      return 0;

    case STATUS_LED_ON:
      led->state = 1;
      status_led_on(led);
      return 0;

    case STATUS_LED_SLOW:
      if (led->state) {
        led->state = 0;
        status_led_off(led);
        return STATUS_LED_SLOW_PERIOD_OFF / portTICK_RATE_MS;
      } else {
        led->state = 1;
        status_led_on(led);
        return STATUS_LED_SLOW_PERIOD_ON / portTICK_RATE_MS;
      }

    case STATUS_LED_FAST:
      if (led->state) {
        led->state = 0;
        status_led_off(led);
      } else {
        led->state = 1;
        status_led_on(led);
      }
      return STATUS_LED_FAST_PERIOD / portTICK_RATE_MS;

    case STATUS_LED_FLASH:
      if (led->state) {
        status_led_off(led);
        return 0;
      } else {
        led->state = 1;
        return STATUS_LED_FLASH_PERIOD / portTICK_RATE_MS;
      }

    case STATUS_LED_PULSE:
      if (led->state) {
        led->state = 0;
        status_led_off(led);
        return STATUS_LED_PULSE_PERIOD_OFF / portTICK_RATE_MS;
      } else {
        led->state = 1;
        status_led_on(led);
        return STATUS_LED_PULSE_PERIOD_ON / portTICK_RATE_MS;
      }

    default:
      return 0;
  }
}

// schedule next tick
static TickType_t status_led_schedule(struct status_led *led, TickType_t period)
{
  TickType_t tick = xTaskGetTickCount();

  if (period == 0) {
    // reset phase
    led->tick = 0;

    // indefinite period
    return portMAX_DELAY;

  } else if (led->tick == 0) {
    // start phase
    led->tick = tick + period;

    return period;

  } else if (led->tick + period > tick) {
    led->tick += period;

    return led->tick - tick;

  } else {
    led->tick += period;

    // missed tick, catchup
    return 0;
  }
}

void status_led_task(void *arg)
{
  struct status_led *led = arg;
  struct status_led_event event;

  led->tick = 0;

  for (;;) {
    TickType_t period = status_led_tick(led);
    TickType_t delay = status_led_schedule(led, period);

    LOG_DEBUG("input=%d mode=%d state=%d -> period=%u delay=%u", led->input, led->mode, led->state, period, delay);

    if (!xQueueReceive(led->queue, &event, delay)) {
      // tick next frame on timeout
      continue;
    }

    switch (event.op) {
      case STATUS_LED_SET:
        status_led_set_mode(led, event.mode);
        break;

      case STATUS_LED_READ:
        status_led_read_mode(led);
        break;
    }
  }
}

int status_led_init_gpio(struct status_led *led, const struct status_led_options options)
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

int status_led_new(struct status_led **ledp, const struct status_led_options options, enum status_led_mode mode)
{
  struct status_led *led;
  int err;

  if (!(led = calloc(1, sizeof(*led)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  led->options = options;
  led->set_mode = mode;

  // set initial state
  status_led_set_mode(led, mode);

  // enable GPIo output
  if ((err = status_led_init_gpio(led, options))) {
    LOG_ERROR("status_led_init_gpio");
    goto error;
  }

  // setup mutex
  if (!(led->mutex = xSemaphoreCreateMutex())) {
    LOG_ERROR("xSemaphoreCreateMutex");
    err = -1;
    goto error;
  }

  // setup task
  if ((led->queue = xQueueCreate(1, sizeof(struct status_led_event))) == NULL) {
    LOG_ERROR("xQueueCreate");
    err = -1;
    goto error;
  }

  if (xTaskCreate(&status_led_task, "status-led", STATUS_LED_TASK_STACK, led, STATUS_LED_TASK_PRIORITY, &led->task) <= 0) {
    LOG_ERROR("xTaskCreate");
    err = -1;
    goto error;
  }

  *ledp = led;

  return 0;

error:
  free(led);

  return err;
}

int status_led_set(struct status_led *led, enum status_led_mode mode)
{
  struct status_led_event event = { STATUS_LED_SET, mode };
  int err = 0;

  if (!xSemaphoreTake(led->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  LOG_DEBUG("gpio=%d: mode=%d", led->options.gpio, mode);

  led->set_mode = mode;

  if (led->override) {
    // skip set op
    LOG_DEBUG("override");

  } else if (xQueueOverwrite(led->queue, &event) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    err = -1;
    goto error;
  }

error:
  if (!xSemaphoreGive(led->mutex)) {
    LOG_WARN("xSemaphoreGive");
  }

  return err;
}

int status_leds_override(struct status_led *led, enum status_led_mode mode)
{
  struct status_led_event event = { STATUS_LED_SET, mode };
  int err = 0;

  if (!xSemaphoreTake(led->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  LOG_DEBUG("gpio=%d: mode=%d", led->options.gpio, mode);

  led->override = true;

  if (xQueueOverwrite(led->queue, &event) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    err = -1;
    goto error;
  }

error:
  if (!xSemaphoreGive(led->mutex)) {
    LOG_WARN("xSemaphoreGive");
  }

  return err;
}

int status_leds_revert(struct status_led *led)
{
  struct status_led_event event = { STATUS_LED_SET, 0 };
  int err = 0;

  if (!xSemaphoreTake(led->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  LOG_DEBUG("gpio=%d: set_mode=%d", led->options.gpio, led->set_mode);

  led->override = false;

  // use last mode set
  event.mode = led->set_mode;

  if (xQueueOverwrite(led->queue, &event) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    err = -1;
    goto error;
  }

error:
  if (!xSemaphoreGive(led->mutex)) {
    LOG_WARN("xSemaphoreGive");
  }

  return err;
}

int status_led_read(struct status_led *led)
{
  struct status_led_event event = { STATUS_LED_READ, 0 };
  uint32_t notify_value;
  int ret = 0;

  if (!xSemaphoreTake(led->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  // request notify
  LOG_DEBUG("gpio=%d: read_task=%p", led->options.gpio, xTaskGetCurrentTaskHandle());

  led->read_task = xTaskGetCurrentTaskHandle();

  // set input mode and read
  if (xQueueOverwrite(led->queue, &event) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    ret = -1;
    goto error;
  }

  // wait
  if (!xTaskNotifyWait(0, STATUS_LED_READ_NOTIFY_BIT, &notify_value, portMAX_DELAY)) {
    LOG_ERROR("xTaskNotifyWait");
    ret = -1;
    goto error;
  }

  LOG_DEBUG("gpio=%d: notify_value=%04x", led->options.gpio, notify_value);

  if (notify_value & STATUS_LED_READ_NOTIFY_BIT) {
    ret = 1;
  }

error:
  if (!xSemaphoreGive(led->mutex)) {
    LOG_WARN("xSemaphoreGive");
  }

  return ret;
}
