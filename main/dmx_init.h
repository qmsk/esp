#pragma once

int init_dmx();

/**
 * Setup the DMX UART, and run any DMX input loop.
 */
int start_dmx();

/**
 * Release the DMX UART.
 */
int restart_dmx();
