#include "gpio.h"

#include <c_types.h>
#include <esp8266/eagle_soc.h>
#include <esp8266/gpio_register.h>

#define GPIO_PIN_BIT(gpio) (1 << (gpio))
#define GPIO_PIN_REG(gpio) ((gpio) * 4)

#define READ_GPIO_REG(reg) READ_PERI_REG(PERIPHS_GPIO_BASEADDR + reg)
#define WRITE_GPIO_REG(reg, value) WRITE_PERI_REG(PERIPHS_GPIO_BASEADDR + reg, value)

#define READ_GPIO_PIN_REG(gpio) READ_PERI_REG(PERIPHS_GPIO_BASEADDR + GPIO_PIN_REG(gpio))
#define WRITE_GPIO_PIN_REG(gpio, value) WRITE_PERI_REG(PERIPHS_GPIO_BASEADDR + GPIO_PIN_REG(gpio), value)

#define GPIO_OUTPUT_HIGH(gpio) WRITE_GPIO_REG(GPIO_OUT_W1TS_ADDRESS, GPIO_PIN_BIT(gpio))
#define GPIO_OUTPUT_LOW(gpio) WRITE_GPIO_REG(GPIO_OUT_W1TC_ADDRESS, GPIO_PIN_BIT(gpio))

void GPIO_SetupOutput(enum GPIO gpio, const struct GPIO_OutputConfig *config)
{
  WRITE_GPIO_REG(GPIO_ENABLE_W1TS_ADDRESS, GPIO_PIN_BIT(gpio));
}

void GPIO_Output(enum GPIO gpio, bool output)
{
  if (output)
    GPIO_OUTPUT_HIGH(gpio);
  else
    GPIO_OUTPUT_LOW(gpio);
}

void GPIO_OutputHigh(enum GPIO gpio)
{
  GPIO_OUTPUT_HIGH(gpio);
}

void GPIO_OutputLow(enum GPIO gpio)
{
  GPIO_OUTPUT_LOW(gpio);
}
