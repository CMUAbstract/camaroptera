#ifndef LIBMSP_UART_H
#define LIBMSP_UART_H

#include <stdint.h>

void msp_uart_open();
void msp_uart_send_sync(uint8_t *payload, unsigned len);
void msp_uart_close();

#endif // LIBMSP_UART_H
