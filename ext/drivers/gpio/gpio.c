#include "gpio.h"

#include <c_types.h>
#include <esp8266/eagle_soc.h>
#include <esp8266/gpio_register.h>

#define WRITE_PERI_REG_MASK(reg, mask, bit) WRITE_PERI_REG((reg), ((READ_PERI_REG(reg) & (~(mask))) | ((bit) ? (mask) : 0)))
#define WRITE_PERI_REG_BITS(reg, mask, bits) WRITE_PERI_REG((reg), ((READ_PERI_REG(reg) & (~(mask)))) | ((bits) & (mask)))

#define GPIO_REG(reg) (PERIPHS_GPIO_BASEADDR + reg)

#define GPIO_PIN_BIT(gpio) (1 << (gpio))
#define GPIO_PIN_REG(gpio) GPIO_REG(GPIO_PIN0_ADDRESS + (gpio) * 4)

#define GPIO_OUTPUT_ENABLE(gpio) WRITE_PERI_REG(GPIO_REG(GPIO_ENABLE_W1TS_ADDRESS), GPIO_PIN_BIT(gpio));
#define GPIO_OUTPUT_HIGH(gpio) WRITE_PERI_REG(GPIO_REG(GPIO_OUT_W1TS_ADDRESS), GPIO_PIN_BIT(gpio))
#define GPIO_OUTPUT_LOW(gpio) WRITE_PERI_REG(GPIO_REG(GPIO_OUT_W1TC_ADDRESS), GPIO_PIN_BIT(gpio))
#define GPIO_OUTPUT(gpio, level) WRITE_PERI_REG_MASK(GPIO_REG(GPIO_OUT_ADDRESS), GPIO_PIN_BIT(gpio), level)

bool GPIO_Exists(enum GPIO gpio)
{
  return (gpio < GPIO_COUNT);
}

void GPIO_SetupOutput(enum GPIO gpio, enum GPIO_OutputMode mode)
{
  GPIO_OUTPUT(gpio, (mode & GPIO_OUTPUT_HIGH));
  GPIO_OUTPUT_ENABLE(gpio);
  WRITE_PERI_REG(GPIO_PIN_REG(gpio),
    ((mode & GPIO_OUTPUT_OPEN_DRAIN) ? GPIO_PIN_DRIVER_MASK : 0)
  );
}

void GPIO_OutputEnable(enum GPIO gpio)
{
  GPIO_OUTPUT_ENABLE(gpio);
}

void GPIO_Output(enum GPIO gpio, bool level)
{
  if (level)
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
