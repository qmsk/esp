#include <uart.h>
#include "../uart.h"

#include <logging.h>

#include <driver/gpio.h>
#include <esp_rom_gpio.h>
#include <soc/uart_periph.h>

static void setup_tx_pin_iomux(uart_port_t port)
{
  const uart_periph_sig_t *uart_periph_sig = &uart_periph_signal[port].pins[SOC_UART_TX_PIN_IDX];

  gpio_iomux_out(uart_periph_sig->default_gpio, uart_periph_sig->iomux_func, false);
}

static void setup_rx_pin_iomux(uart_port_t port)
{
  const uart_periph_sig_t *uart_periph_sig = &uart_periph_signal[port].pins[SOC_UART_RX_PIN_IDX];

  gpio_iomux_out(uart_periph_sig->default_gpio, uart_periph_sig->iomux_func, false);
  gpio_iomux_in(uart_periph_sig->default_gpio, uart_periph_sig->signal);
}

static void setup_tx_pin_gpio(uart_port_t port, gpio_num_t gpio)
{
  gpio_set_level(gpio, 1);
  gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
  gpio_iomux_out(gpio, PIN_FUNC_GPIO, false);

  esp_rom_gpio_connect_out_signal(gpio, UART_PERIPH_SIGNAL(port, SOC_UART_TX_PIN_IDX), false, false);
}

static void setup_rx_pin_gpio(uart_port_t port, gpio_num_t gpio)
{
  gpio_iomux_out(gpio, PIN_FUNC_GPIO, false);
  gpio_set_direction(gpio, GPIO_MODE_INPUT);
  gpio_set_pull_mode(gpio, GPIO_PULLUP_ONLY);

  esp_rom_gpio_connect_in_signal(gpio, UART_PERIPH_SIGNAL(port, SOC_UART_RX_PIN_IDX), false);
}

int uart_pin_setup(struct uart *uart, struct uart_options options)
{
  uart_port_t port = uart->port & UART_PORT_MASK;
  int err = 0;

  if (options.pin_mutex) {
    LOG_DEBUG("wait pin_mutex=%p", options.pin_mutex);

    if (!xSemaphoreTake(options.pin_mutex, portMAX_DELAY)) {
      LOG_ERROR("xSemaphoreTake");
      return -1;
    } else {
      uart->pin_mutex = options.pin_mutex;
    }
  }

  LOG_DEBUG("port=%d: rx_pin=%d tx_pin=%d", port, options.rx_pin, options.tx_pin);

  taskENTER_CRITICAL(&uart->mux);

  if (options.rx_pin == 0) {
    // XXX: not compatible with flash qio mode?
    setup_rx_pin_iomux(port);

  } else if (options.rx_pin > 0) {
    setup_rx_pin_gpio(port, options.rx_pin);
  }

  if (options.tx_pin == 0) {
    // XXX: not compatible with flash qio mode?
    setup_tx_pin_iomux(port);

  } else if (options.tx_pin > 0) {
    // via GPIO matrix
    setup_tx_pin_gpio(port, options.tx_pin);
  }

  taskEXIT_CRITICAL(&uart->mux);

  return err;
}

void uart_pin_teardown(struct uart *uart)
{
  if (uart->pin_mutex) {
    xSemaphoreGive(uart->pin_mutex);

    uart->pin_mutex = NULL;
  }
}
