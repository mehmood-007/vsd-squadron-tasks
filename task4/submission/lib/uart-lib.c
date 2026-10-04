#include <uart-lib.h>

void usart1_init(void){
    // Enable clocks for GPIOD and USART1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitTypeDef gpio = {0};

    // TX: PD5 (default USART1_TX for CH32V003)
    gpio.GPIO_Pin = GPIO_Pin_5;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &gpio);

    // RX: PD6 (default USART1_RX for CH32V003)
    gpio.GPIO_Pin = GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &gpio);

    USART_InitTypeDef usart = {0};
    usart.USART_BaudRate = USART_BAUD_RATE;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    USART_Init(USART_PORT, &usart);
    USART_Cmd(USART_PORT, ENABLE);
}





void usart1_write_2byte(uint16_t data){
    while (USART_GetFlagStatus(USART_PORT, USART_FLAG_TXE) == RESET)
    {}

    USART_SendData(USART_PORT, data);
}

uint16_t usart1_read_2byte(void){
    while (USART_GetFlagStatus(USART_PORT, USART_FLAG_RXNE) == RESET)
    {}

    return (uint16_t)USART_ReceiveData(USART_PORT);
}

void usart1_write_uint(uint32_t v){
    char buf[11];
    uint8_t i = 0;
    if (v == 0){ usart1_write_2byte('0'); return; }
    while (v){ buf[i++] = (char)('0' + (v % 10u)); v /= 10u; }
    while (i--){ usart1_write_2byte((uint16_t)buf[i]); }
}

uint8_t usart1_available(void){
    return (USART_GetFlagStatus(USART_PORT, USART_FLAG_RXNE) == SET) ? 1 : 0;
}

// Reads until '\r', '\n', or maxLen-1 chars; null-terminates buf.
void usart1_read_string(char *buf, uint16_t maxLen){
    uint16_t i = 0;
    if (buf == 0 || maxLen == 0) return;

    while (i < (uint16_t)(maxLen - 1)){
        char c = (char)usart1_read_2byte();
        if (c == '\r' || c == '\n'){
            break;
        }
        buf[i++] = c;
    }
    buf[i] = '\0';
}

/*
void usart1_write_string(const char *str){
    while (*str){
        usart1_write_byte((uint16_t)*str++);
    }

    while (USART_GetFlagStatus(USART_PORT, USART_FLAG_TC) == RESET){
    }
}
*/

void usart1_write_string(const char *str){
    while (*str){
        usart1_write_2byte((uint16_t)*str++);
    }

    while (USART_GetFlagStatus(USART_PORT, USART_FLAG_TC) == RESET){
    }
}
