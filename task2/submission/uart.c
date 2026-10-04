#include "uart.h"
#include <ch32v00x.h>

void uart_init(uint32_t baud){
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitTypeDef gpio = {0};
    gpio.GPIO_Pin   = GPIO_Pin_5;   // TX
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &gpio);

    gpio.GPIO_Pin  = GPIO_Pin_6;    // RX
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &gpio);

    USART_InitTypeDef usart = {0};
    usart.USART_BaudRate            = baud;
    usart.USART_WordLength          = USART_WordLength_8b;
    usart.USART_StopBits            = USART_StopBits_1;
    usart.USART_Parity              = USART_Parity_No;
    usart.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &usart);
    USART_Cmd(USART1, ENABLE);
}

void uart_write_char(char c){
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET){}
    USART_SendData(USART1, (uint16_t)c);
}

void uart_write_string(const char *s){
    while (*s){
        uart_write_char(*s++);
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET){}
}

void uart_write_uint(uint32_t v){
    char buf[10];
    uint8_t i = 0;
    do {
        buf[i++] = (char)('0' + (v % 10u));
        v /= 10u;
    } while (v);
    while (i){
        uart_write_char(buf[--i]);
    }
}
