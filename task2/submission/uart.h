#ifndef UART_H
#define UART_H

#include <stdint.h>

// USART1 on CH32V003: TX = PD5, RX = PD6, 8N1.
void uart_init(uint32_t baud);
void uart_write_char(char c);
void uart_write_string(const char *s);
void uart_write_uint(uint32_t v);

#endif // UART_H
