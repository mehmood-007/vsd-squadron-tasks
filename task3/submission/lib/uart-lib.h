#ifndef __UART_LIB_SRC_UART_LIB_H
#define __UART_LIB_SRC_UART_LIB_H

#include <stdint.h>

// USART1 on CH32V003: TX = PD5, RX = PD6, 8N1, no flow control.
// RX is interrupt-driven into a ring buffer; the USART itself has no RX FIFO.
#define UART_RX_BUF_SIZE 64   // must be a power of 2

void    uart_init(uint32_t baud);

void    uart_write_byte(uint8_t c);
void    uart_write_string(const char *s);
void    uart_write_uint(uint32_t v);
void    uart_write_hex8(uint8_t v);

uint8_t uart_available(void);
uint8_t uart_try_read(uint8_t *c);
uint8_t uart_read_byte(void);

#endif // __UART_LIB_SRC_UART_LIB_H