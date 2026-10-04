#include <uart-lib.h>
#include <ch32v00x.h>

void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

static volatile uint8_t rx_buf[UART_RX_BUF_SIZE];
static volatile uint8_t rx_head;
static volatile uint8_t rx_tail;

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

    rx_head = rx_tail = 0;
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    NVIC_InitTypeDef nvic = {0};
    nvic.NVIC_IRQChannel                   = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority        = 1;
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);

    USART_Cmd(USART1, ENABLE);
}

void uart_write_byte(uint8_t c){
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET){}
    USART_SendData(USART1, c);
}

void uart_write_string(const char *s){
    while (*s){
        uart_write_byte((uint8_t)*s++);
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
        uart_write_byte((uint8_t)buf[--i]);
    }
}

void uart_write_hex8(uint8_t v){
    static const char hex[] = "0123456789ABCDEF";
    uart_write_byte((uint8_t)hex[v >> 4]);
    uart_write_byte((uint8_t)hex[v & 0x0F]);
}

uint8_t uart_available(void){
    return rx_head != rx_tail;
}

// Non-blocking: returns 1 and stores the byte in *c if one is buffered, else 0.
uint8_t uart_try_read(uint8_t *c){
    if (rx_head == rx_tail) return 0;
    *c = rx_buf[rx_tail];
    rx_tail = (uint8_t)((rx_tail + 1u) & (UART_RX_BUF_SIZE - 1u));
    return 1;
}

uint8_t uart_read_byte(void){
    uint8_t c;
    while (!uart_try_read(&c)){}
    return c;
}

// Bytes arriving while the buffer is full are dropped.
void USART1_IRQHandler(void){
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET){
        uint8_t c = (uint8_t)USART_ReceiveData(USART1);
        uint8_t next = (uint8_t)((rx_head + 1u) & (UART_RX_BUF_SIZE - 1u));
        if (next != rx_tail){
            rx_buf[rx_head] = c;
            rx_head = next;
        }
    }
}
