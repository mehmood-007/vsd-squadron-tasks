
#include <gpio-lib.h>

void gpio_init(void){
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,  ENABLE);
    GPIO_InitTypeDef gpio = {0};

    // TX: PD5 (default USART1_TX for CH32V003)
    gpio.GPIO_Pin = GPIO_PIN_BLINK;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIO_PORT_BLINK, &gpio);
}
void gpio_set( uint8_t ledState ){
    	GPIO_WriteBit(GPIO_PORT_BLINK, GPIO_PIN_BLINK, ledState);
}
void gpio_clear(void){
    	GPIO_WriteBit(GPIO_PORT_BLINK, GPIO_PIN_BLINK, Bit_RESET);
}

void toggle_blink_led(void){
    static uint8_t ledState = 0;
    ledState = !ledState;
    gpio_set(ledState);
}

void button_init(void){
    RCC_APB2PeriphClockCmd(BTN_PORT_RCC, ENABLE);
    GPIO_InitTypeDef g = {0};
    g.GPIO_Pin   = BTN_PIN;
    g.GPIO_Mode  = BTN_ACTIVE_HIGH ? GPIO_Mode_IPD : GPIO_Mode_IPU;
    g.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(BTN_PORT, &g);
}

uint8_t button_pressed(void){
    return GPIO_ReadInputDataBit(BTN_PORT, BTN_PIN) == (BTN_ACTIVE_HIGH ? Bit_SET : Bit_RESET);
}

uint8_t button_pressed_debounced(void){
    if (!button_pressed()) return 0;
    Delay_Ms(BTN_DEBOUNCE_MS);
    return button_pressed();
}

// Returns once the button has read released continuously for BTN_DEBOUNCE_MS.
void button_wait_release(void){
    uint8_t stable = 0;
    while (stable < BTN_DEBOUNCE_MS){
        stable = button_pressed() ? 0 : stable + 1;
        Delay_Ms(1);
    }
}

static GPIO_TypeDef *port_from_char(char c){
    switch (c){
        case 'A': case 'a': return GPIOA;
        case 'C': case 'c': return GPIOC;
        case 'D': case 'd': return GPIOD;
        default: return 0;
    }
}

static uint32_t rcc_from_char(char c){
    switch (c){
        case 'A': case 'a': return RCC_APB2Periph_GPIOA;
        case 'C': case 'c': return RCC_APB2Periph_GPIOC;
        case 'D': case 'd': return RCC_APB2Periph_GPIOD;
        default: return 0;
    }
}

static uint8_t parse_pin(const char *s, GPIO_TypeDef **port, uint16_t *pin_mask){
    if (!s || !s[0] || !s[1] || s[2]) return 0;
    GPIO_TypeDef *p = port_from_char(s[0]);
    if (!p) return 0;
    if (s[1] < '0' || s[1] > '7') return 0;
    *port = p;
    *pin_mask = (uint16_t)(1u << (s[1] - '0'));
    return 1;
}

int8_t gpio_read_pin(const char *pin){
    GPIO_TypeDef *port;
    uint16_t mask;
    if (!parse_pin(pin, &port, &mask)) return -1;

    RCC_APB2PeriphClockCmd(rcc_from_char(pin[0]), ENABLE);
    GPIO_InitTypeDef g = {0};
    g.GPIO_Pin   = mask;
    g.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    g.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(port, &g);
    return (int8_t)GPIO_ReadInputDataBit(port, mask);
}
