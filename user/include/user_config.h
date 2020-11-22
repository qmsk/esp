#ifndef __USER__CONFIG_H__
#define __USER__CONFIG_H__

#include <lib/config.h>

#define USER_CONFIG_UART_BAUD_RATE 74880

extern struct config user_config;

int init_config(struct config *config);

#endif
