#include <status_led.h>

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <logging.h>

// 100ms on / 1800ms off
#define STATUS_LED_BLINK_SLOW_PERIOD_ON 100
#define STATUS_LED_BLINK_SLOW_PERIOD_OFF 1800

// 100ms on / 100ms off
#define STATUS_LED_BLINK_FAST_PERIOD 100

// 100ms on
#define STATUS_LED_BLINK_PERIOD 10

#define STATUS_LED_TASK_STACK 256
#define STATUS_LED_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

struct status_led {
  struct status_led_options options;

  xTaskHandle task;
  xQueueHandle queue;
  portTickType tick;

  enum status_led_mode mode;
  unsigned state;
};

static inline void status_led_off(struct status_led *led)
{
  gpio_set_level(led->options.gpio, led->options.inverted ? 1 : 0);
}

static inline void status_led_on(struct status_led *led)
{
  gpio_set_level(led->options.gpio, led->options.inverted ? 0 : 1);
}

static void status_led_update(struct status_led *led, enum status_led_mode mode)
{
  switch(led->mode = mode) {
    case STATUS_LED_OFF:
      status_led_off(led);
      break;

    case STATUS_LED_ON:
      status_led_on(led);
      break;

    case STATUS_LED_SLOW:
    case STATUS_LED_FAST:
    case STATUS_LED_FLASH:
      led->state = 0;
      status_led_on(led);
      break;
  }
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

    default:
      return 0;
  }
}

// schedule next tick
static portTickType status_led_schedule(struct status_led *led, portTickType period)
{
  portTickType tick = xTaskGetTickCount();

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
    portTickType period = status_led_tick(led);
    portTickType delay = status_led_schedule(led, period);

    if (xQueueReceive(led->queue, &mode, delay)) {
      LOG_DEBUG("xQueueReceive -> update")
      status_led_update(led, mode);
    } else {
      LOG_DEBUG("xQueueReceive -> tick");
    }
  }
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

  gpio_set_direction(options.gpio, GPIO_MODE_OUTPUT);

  // set initial state
  status_led_update(led, mode);

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
  if (xQueueOverwrite(led->queue, &mode) <= 0) {
    LOG_ERROR("xQueueOverwrite");
    return -1;
  }

  return 0;
}
