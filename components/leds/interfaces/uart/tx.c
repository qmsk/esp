#include "../uart.h"
#include "../../leds.h"
#include "../../stats.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define UART_TX_TASK_PRIORITY 14

int leds_interface_uart_init(struct leds_interface_uart *interface, const struct leds_interface_uart_options *options, enum leds_interface_uart_mode mode, union leds_interface_uart_func func)
{
  interface->mode = mode;
  interface->func = func;

  interface->uart = options->uart;

  switch (mode) {
    case LEDS_INTERFACE_UART_MODE_24B3I7_0U4_80U:
      interface->uart_options = (struct uart_options) {
        .baud_rate    = UART_BAUD_2500000,
        .data_bits    = UART_DATA_7_BITS,
        .parity_bits  = UART_PARITY_DISABLE,
        .stop_bits    = UART_STOP_BITS_1,

        .tx_inverted  = true
      };
      break;

    case LEDS_INTERFACE_UART_MODE_24B2I8_0U25_50U:
      interface->uart_options = (struct uart_options) {
        .baud_rate    = UART_BAUD_4000000,
        .data_bits    = UART_DATA_8_BITS,
        .parity_bits  = UART_PARITY_DISABLE,
        .stop_bits    = UART_STOP_BITS_1,

        .tx_inverted  = true
      };
      break;

    case LEDS_INTERFACE_UART_MODE_32B2I6_0U3_80U:
      interface->uart_options = (struct uart_options) {
        .baud_rate    = UART_BAUD_3333333,
        .data_bits    = UART_DATA_6_BITS,
        .parity_bits  = UART_PARITY_DISABLE,
        .stop_bits    = UART_STOP_BITS_1,

        .tx_inverted  = true
      };
      break;

    default:
      LOG_FATAL("invalid mode=%d", mode);
  }

  interface->uart_options.pin_mutex = options->pin_mutex;

  interface->gpio_options = options->gpio.gpio_options;
  interface->gpio_out_pins = options->gpio.gpio_out_pins;

  return 0;
}

int leds_interface_uart_tx_pixel(struct leds_interface_uart *interface, const struct leds_color *pixels, unsigned index, const struct leds_limit *limit)
{
  switch (interface->mode) {
    case LEDS_INTERFACE_UART_MODE_24B3I7_0U4_80U:
      interface->func.uart_mode_24B3I7(interface->buf.uart_mode_24B3I7, pixels, index, limit);

      return uart_write_all(interface->uart, interface->buf.uart_mode_24B3I7, sizeof(interface->buf.uart_mode_24B3I7));

    case LEDS_INTERFACE_UART_MODE_24B2I8_0U25_50U:
      interface->func.uart_mode_24B2I8(interface->buf.uart_mode_24B2I8, pixels, index, limit);

      return uart_write_all(interface->uart, interface->buf.uart_mode_24B2I8, sizeof(interface->buf.uart_mode_24B2I8));

    case LEDS_INTERFACE_UART_MODE_32B2I6_0U3_80U:
      interface->func.uart_mode_32B2I6(interface->buf.uart_mode_32B2I6, pixels, index, limit);

      return uart_write_all(interface->uart, interface->buf.uart_mode_32B2I6, sizeof(interface->buf.uart_mode_32B2I6));

    default:
      LOG_FATAL("invalid mode=%d", interface->mode);
  }
}

int leds_interface_uart_tx_reset(struct leds_interface_uart *interface)
{
  switch (interface->mode) {
    case LEDS_INTERFACE_UART_MODE_24B3I7_0U4_80U:
      return uart_mark(interface->uart, 80);

    case LEDS_INTERFACE_UART_MODE_24B2I8_0U25_50U:
      return uart_mark(interface->uart, 50);

    case LEDS_INTERFACE_UART_MODE_32B2I6_0U3_80U:
      return uart_mark(interface->uart, 80);

    default:
      LOG_FATAL("invalid mode=%d", interface->mode);
  }
}

int leds_interface_uart_tx(struct leds_interface_uart *interface, const struct leds_color *pixels, unsigned count, const struct leds_limit *limit)
{
  struct leds_interface_uart_stats *stats = &leds_interface_stats.uart;
  UBaseType_t task_priority = uxTaskPriorityGet(NULL);
  int err;

  WITH_STATS_TIMER(&stats->open) {
    if ((err = uart_open(interface->uart, interface->uart_options))) {
      LOG_ERROR("uart_open");
      return err;
    }
  }

#if CONFIG_LEDS_GPIO_ENABLED
  if (interface->gpio_options) {
    gpio_out_set(interface->gpio_options, interface->gpio_out_pins);
  }
#endif

  WITH_STATS_TIMER(&stats->tx) {
    // temporarily raise task priority to ensure uart TX buffer does not starve
    vTaskPrioritySet(NULL, UART_TX_TASK_PRIORITY);

    for (unsigned i = 0; i < count; i++) {
      if ((err = leds_interface_uart_tx_pixel(interface, pixels, i, limit))) {
        LOG_ERROR("leds_interface_uart_tx_pixel");
        goto error;
      }
    }

    // restore previous task priority
    vTaskPrioritySet(NULL, task_priority);

    if ((err = leds_interface_uart_tx_reset(interface))) {
      LOG_ERROR("leds_interface_uart_tx_reset");
      goto error;
    }
  }

error:
#if CONFIG_LEDS_GPIO_ENABLED
  if (interface->gpio_options) {
    gpio_out_clear(interface->gpio_options);
  }
#endif

  if ((err = uart_close(interface->uart))) {
    LOG_ERROR("uart_close");
    return err;
  }

  return err;
}
