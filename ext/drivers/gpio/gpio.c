#include "gpio.h"

#include <c_types.h>
#include <esp8266/eagle_soc.h>
#include <esp8266/gpio_register.h>
#include <esp8266/pin_mux_register.h>

#define WRITE_PERI_REG_MASK(reg, mask, bit) WRITE_PERI_REG((reg), ((READ_PERI_REG(reg) & (~(mask))) | ((bit) ? (mask) : 0)))
#define WRITE_PERI_REG_BITS(reg, mask, bits) WRITE_PERI_REG((reg), ((READ_PERI_REG(reg) & (~(mask)))) | ((bits) & (mask)))

#define PIN_FUNC_BITS(func) (((((func) & 0x4) << 2) | ((func) & 0x3)) << PERIPHS_IO_MUX_FUNC_S)

#define GPIO_REG(reg) (PERIPHS_GPIO_BASEADDR + reg)

#define GPIO_PIN_BIT(gpio) (1 << (gpio))
#define GPIO_PIN_REG(gpio) GPIO_REG(GPIO_PIN0_ADDRESS + (gpio) * 4)

#define GPIO_OUTPUT_ENABLE(gpio) WRITE_PERI_REG(GPIO_REG(GPIO_ENABLE_W1TS_ADDRESS), GPIO_PIN_BIT(gpio));
#define GPIO_OUTPUT_HIGH(gpio) WRITE_PERI_REG(GPIO_REG(GPIO_OUT_W1TS_ADDRESS), GPIO_PIN_BIT(gpio))
#define GPIO_OUTPUT_LOW(gpio) WRITE_PERI_REG(GPIO_REG(GPIO_OUT_W1TC_ADDRESS), GPIO_PIN_BIT(gpio))
#define GPIO_OUTPUT(gpio, level) WRITE_PERI_REG_MASK(GPIO_REG(GPIO_OUT_ADDRESS), GPIO_PIN_BIT(gpio), level)

static const uint32_t gpio_func_reg_map[GPIO_COUNT] = {
  [GPIO_0]  = PERIPHS_IO_MUX_GPIO0_U,
  [GPIO_1]  = PERIPHS_IO_MUX_U0TXD_U,
  [GPIO_2]  = PERIPHS_IO_MUX_GPIO2_U,
  [GPIO_3]  = PERIPHS_IO_MUX_U0RXD_U,
  [GPIO_4]  = PERIPHS_IO_MUX_GPIO4_U,
  [GPIO_5]  = PERIPHS_IO_MUX_GPIO5_U,
  [GPIO_6]  = PERIPHS_IO_MUX_SD_CLK_U,
  [GPIO_7]  = PERIPHS_IO_MUX_SD_DATA0_U,
  [GPIO_8]  = PERIPHS_IO_MUX_SD_DATA1_U,
  [GPIO_9]  = PERIPHS_IO_MUX_SD_DATA2_U,
  [GPIO_10] = PERIPHS_IO_MUX_SD_DATA3_U,
  [GPIO_11] = PERIPHS_IO_MUX_SD_CMD_U,
  [GPIO_12] = PERIPHS_IO_MUX_MTDI_U,
  [GPIO_13] = PERIPHS_IO_MUX_MTCK_U,
  [GPIO_14] = PERIPHS_IO_MUX_MTMS_U,
  [GPIO_15] = PERIPHS_IO_MUX_MTDO_U,
};

static const uint32_t gpio_func_bits_map[GPIO_COUNT] = {
  [GPIO_0]  = PIN_FUNC_BITS(FUNC_GPIO0),
  [GPIO_1]  = PIN_FUNC_BITS(FUNC_GPIO1),
  [GPIO_2]  = PIN_FUNC_BITS(FUNC_GPIO2),
  [GPIO_3]  = PIN_FUNC_BITS(FUNC_GPIO3),
  [GPIO_4]  = PIN_FUNC_BITS(FUNC_GPIO4),
  [GPIO_5]  = PIN_FUNC_BITS(FUNC_GPIO5),
  [GPIO_6]  = PIN_FUNC_BITS(FUNC_GPIO6),
  [GPIO_7]  = PIN_FUNC_BITS(FUNC_GPIO7),
  [GPIO_8]  = PIN_FUNC_BITS(FUNC_GPIO8),
  [GPIO_9]  = PIN_FUNC_BITS(FUNC_GPIO9),
  [GPIO_10] = PIN_FUNC_BITS(FUNC_GPIO10),
  [GPIO_11] = PIN_FUNC_BITS(FUNC_GPIO11),
  [GPIO_12] = PIN_FUNC_BITS(FUNC_GPIO12),
  [GPIO_13] = PIN_FUNC_BITS(FUNC_GPIO13),
  [GPIO_14] = PIN_FUNC_BITS(FUNC_GPIO14),
  [GPIO_15] = PIN_FUNC_BITS(FUNC_GPIO15),
};

bool GPIO_Exists(enum GPIO gpio)
{
  return (gpio < GPIO_COUNT);
}

static inline void GPIO_SelectPinFunc(enum GPIO gpio, uint32 flags)
{
  WRITE_PERI_REG(gpio_func_reg_map[gpio], gpio_func_bits_map[gpio] | flags);
}

static inline void GPIO_SetPin(enum GPIO gpio, uint32_t bits)
{
  WRITE_PERI_REG(GPIO_PIN_REG(gpio), bits);
}

inline void GPIO_OutputEnable(enum GPIO gpio)
{
  GPIO_OUTPUT_ENABLE(gpio);
}

inline void GPIO_Output(enum GPIO gpio, bool level)
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

void GPIO_SetupOutput(enum GPIO gpio, enum GPIO_OutputMode mode)
{
  GPIO_Output(gpio, (mode & GPIO_OUTPUT_HIGH));
  GPIO_SetPin(gpio,
    ((mode & GPIO_OUTPUT_OPEN_DRAIN) ? GPIO_PIN_DRIVER_MASK : 0)
  );
  GPIO_OutputEnable(gpio);
  GPIO_SelectPinFunc(gpio, 0);
}
