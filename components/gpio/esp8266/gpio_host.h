#include <gpio.h>

#include <esp8266/eagle_soc.h>
#include <esp8266/gpio_struct.h>
#include <esp8266/pin_mux_register.h>

// glitchless version of PIN_FUNC_SELECT(), does not reset pin func to default if already set
#define IDEMPOTENT_PIN_FUNC_SELECT(PIN_NAME, FUNC) \
  SET_PERI_REG_BITS(PIN_NAME, PERIPHS_IO_MUX_FUNC, ((FUNC & 0x4) << 2) | (FUNC & 0x3), PERIPHS_IO_MUX_FUNC_S)

#define RTC_GPIO_BIT 0x1

static inline void gpio_rtc_setup_pin()
{
  WRITE_PERI_REG(PAD_XPD_DCDC_CONF, ((READ_PERI_REG(PAD_XPD_DCDC_CONF) & (uint32_t)0xffffffbc)) | (uint32_t)0x1); 	// mux configuration for XPD_DCDC and rtc_gpio0 connection
  CLEAR_PERI_REG_MASK(RTC_GPIO_CONF, 0x1);    //mux configuration for out enable
}

static inline gpio_pins_t gpio_host_get_pins(gpio_pins_t pins)
{
  gpio_pins_t ret = 0;

  if (pins & GPIO_HOST_PINS_HOST) {
    ret |= GPIO.in & pins;
  }
  if (pins & GPIO_HOST_PINS_RTC) {
    ret |= (READ_PERI_REG(RTC_GPIO_IN_DATA) & RTC_GPIO_BIT) ? GPIO_HOST_PINS_RTC : 0;
  }

  return ret;
}

static inline void gpio_host_setup_pins(gpio_pins_t pins, gpio_pins_t outputs)
{
  if (pins & GPIO_HOST_PINS_HOST) {
    GPIO.enable_w1ts      = (uint32_t)(pins & GPIO_HOST_PINS_HOST & outputs);
    GPIO.enable_w1tc      = (uint32_t)(pins & GPIO_HOST_PINS_HOST & ~outputs);
  }

  if (pins & GPIO_HOST_PINS_RTC) {
    if (outputs & GPIO_HOST_PINS_RTC) {
      SET_PERI_REG_MASK(RTC_GPIO_ENABLE, RTC_GPIO_BIT);
    } else {
      CLEAR_PERI_REG_MASK(RTC_GPIO_ENABLE, RTC_GPIO_BIT);
    }
  }
}

static inline void gpio_host_set_pins(gpio_pins_t pins, gpio_pins_t levels)
{
  if (pins & GPIO_HOST_PINS_HOST) {
    GPIO.out_w1ts = (pins & GPIO_HOST_PINS_HOST & levels);
    GPIO.out_w1tc = (pins & GPIO_HOST_PINS_HOST & ~levels);
  }

  if (pins & GPIO_HOST_PINS_RTC) {
    if (levels & GPIO_HOST_PINS_RTC) {
      SET_PERI_REG_MASK(RTC_GPIO_OUT, RTC_GPIO_BIT);
    } else {
      CLEAR_PERI_REG_MASK(RTC_GPIO_OUT, RTC_GPIO_BIT);
    }
  }
}
