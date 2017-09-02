#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdbool.h>

enum GPIO {
  GPIO_0 = 0,
  GPIO_1,
  GPIO_2,
  GPIO_3,
  GPIO_4,
  GPIO_5,
  GPIO_6,
  GPIO_7,
  GPIO_8,
  GPIO_9,
  GPIO_10,
  GPIO_11,
  GPIO_12,
  GPIO_13,
  GPIO_14,
  GPIO_15,
};

struct GPIO_OutputConfig {

};

void GPIO_SetupOutput(enum GPIO gpio, const struct GPIO_OutputConfig *config);

void GPIO_Output(enum GPIO gpio, bool output);
void GPIO_OutputHigh(enum GPIO gpio);
void GPIO_OutputLow(enum GPIO gpio);

#endif
