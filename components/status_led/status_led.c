#include <status_led.h>
#include "status_led.h"

#include <logging.h>

#include <driver/gpio.h>
#include <esp8266/gpio_struct.h>
#include <esp_err.h>

// 100ms on / 1800ms off
#define STATUS_LED_BLINK_SLOW_PERIOD_ON 100
#define STATUS_LED_BLINK_SLOW_PERIOD_OFF 1800

// 100ms on / 100ms off
#define STATUS_LED_BLINK_FAST_PERIOD 100

// 100ms on
#define STATUS_LED_BLINK_PERIOD 10

// 10ms read wait
#define STATUS_LED_READ_WAIT_PERIOD 1
#define STATUS_LED_READ_NOTIFY_BIT 0x1

#if DEBUG
# define STATUS_LED_TASK_STACK 1024
#else
# define STATUS_LED_TASK_STACK 512
#endif
#define STATUS_LED_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

static inline void status_led_output_mode(struct status_led *led)
{
  LOG_DEBUG("gpio_set_direction(%d, GPIO_MODE_OUTPUT)", led->options.gpio);

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
  LOG_DEBUG("gpio_set_direction(%d, GPIO_MODE_INPUT)", led->options.gpio);

  gpio_set_direction(led->options.gpio, GPIO_MODE_INPUT);
}

static inline int status_led_input(struct status_led *led)
{
  if (led->options.gpio == 1) {
    LOG_DEBUG("READ_PERI_REG(PERIPHS_IO_MUX_U0TXD_U)=%08x", READ_PERI_REG(PERIPHS_IO_MUX_U0TXD_U));
  }

  int level = gpio_get_level(led->options.gpio);

  LOG_DEBUG("gpio_get_level(%d) -> %d", led->options.gpio, level);

  return led->options.inverted ? !level : level;
}

static void status_led_update(struct status_led *led, enum status_led_mode mode)
{
  LOG_DEBUG("mode=%d", mode);

  switch(led->mode = mode) {
    case STATUS_LED_OFF:
      status_led_output_mode(led);
      status_led_off(led);
      break;

    case STATUS_LED_ON:
      status_led_output_mode(led);
      status_led_on(led);
      break;

    case STATUS_LED_SLOW:
    case STATUS_LED_FAST:
    case STATUS_LED_FLASH:
      led->state = 0;
      status_led_output_mode(led);
      status_led_on(led);
      break;

    case STATUS_LED_READ:
      led->state = 0;
      status_led_off(led);
      status_led_input_mode(led);
      break;
  }
}

static void status_led_notify(struct status_led *led, int level)
{
  LOG_DEBUG("gpio=%d: level=%d", led->options.gpio, level);

  xTaskNotify(led->read_task, level ? STATUS_LED_READ_NOTIFY_BIT : 0, eSetBits);
}

static portTickType status_led_tick(struct status_led *led)
{
  switch(led->mode) {
    case STATUS_LED_OFF:
    case STATUS_LED_ON:
      return 0;

    case STATUS_LED_SLOW:
      if (led->state) {
        led->state = 0;
        status_led_off(led);
        return STATUS_LED_BLINK_SLOW_PERIOD_OFF / portTICK_RATE_MS;
      } else {
        led->state = 1;
        status_led_on(led);
        return STATUS_LED_BLINK_SLOW_PERIOD_ON / portTICK_RATE_MS;
      }

    case STATUS_LED_FAST:
      if (led->state) {
        led->state = 0;
        status_led_off(led);
      } else {
        led->state = 1;
        status_led_on(led);
      }
      return STATUS_LED_BLINK_FAST_PERIOD / portTICK_RATE_MS;

    case STATUS_LED_FLASH:
      if (led->state) {
        status_led_off(led);
        return 0;
      } else {
        led->state = 1;
        return STATUS_LED_BLINK_PERIOD / portTICK_RATE_MS;
      }

    case STATUS_LED_READ:
      if (led->state) {
        // read input and notify waiting task
        status_led_notify(led, status_led_input(led));
        return 0;
      } else {
        // wait for input to settle
        led->state = 1;
        return STATUS_LED_READ_WAIT_PERIOD;
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
  enum status_led_mode mode;

  led->tick = 0;

  for (;;) {
    TickType_t period = status_led_tick(led);
    TickType_t delay = status_led_schedule(led, period);

    LOG_DEBUG("mode=%d state=%d -> period=%u delay=%u", led->mode, led->state, period, delay);

    if (xQueueReceive(led->queue, &mode, delay)) {
      status_led_update(led, mode);
    }
  }
}

int status_led_init_gpio(struct status_led *led, const struct status_led_options options)
{
  gpio_config_t config = {
      .pin_bit_mask = (1 << options.gpio),
      .mode         = GPIO_MODE_OUTPUT,
      .pull_up_en   = options.inverted ? 1 : 0,
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

  // set initial state
  status_led_update(led, mode);

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
  if ((led->queue = xQueueCreate(1, sizeof(enum status_led_mode))) == NULL) {
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

int status_led_mode(struct status_led *led, enum status_led_mode mode)
{
  int err = 0;

  LOG_DEBUG("gpio=%d: mode=%d", led->options.gpio, mode);

  if (!xSemaphoreTake(led->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  if (xQueueOverwrite(led->queue, &mode) <= 0) {
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
  enum status_led_mode mode = STATUS_LED_READ;
  uint32_t notify_value;
  int ret = 0;

  LOG_DEBUG("gpio=%d: read...", led->options.gpio);

  if (!xSemaphoreTake(led->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  // request notify
  led->read_task = xTaskGetCurrentTaskHandle();

  // set input mode and read
  if (xQueueOverwrite(led->queue, &mode) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    ret = -1;
    goto error;
  }

  // wait
  if (!xTaskNotifyWait(STATUS_LED_READ_NOTIFY_BIT, STATUS_LED_READ_NOTIFY_BIT, &notify_value, portMAX_DELAY)) {
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
