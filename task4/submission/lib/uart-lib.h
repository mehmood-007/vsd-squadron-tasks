#ifndef __UART_LIB_SRC_UART_LIB_H
#define __UART_LIB_SRC_UART_LIB_H

#include <ch32v00x.h>
#include <debug.h>

#define USART_PORT USART1
#define USART_BAUD_RATE 115200

void usart1_init(void);
void usart1_write_2byte(uint16_t data);
uint16_t usart1_read_2byte(void);
uint8_t  usart1_available(void);
void usart1_write_string(const char *str);
void usart1_write_uint(uint32_t v);
void usart1_read_string(char *buf, uint16_t maxLen);

#endif // __UART_LIB_SRC_UART_LIB_H