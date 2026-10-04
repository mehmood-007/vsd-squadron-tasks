#include <led-lib.h>
#include <ch32v00x.h>

#define LED_PORT GPIOC
#define LED_PIN  GPIO_Pin_6

static uint8_t led_state;

void led_init(void){
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitTypeDef gpio = {0};
    gpio.GPIO_Pin   = LED_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_PORT, &gpio);
    led_off();
}

void led_on(void){
    GPIO_WriteBit(LED_PORT, LED_PIN, Bit_SET);
    led_state = 1;
}

void led_off(void){
    GPIO_WriteBit(LED_PORT, LED_PIN, Bit_RESET);
    led_state = 0;
}

void led_toggle(void){
    if (led_state) led_off(); else led_on();
}

uint8_t led_get(void){
    return led_state;
}
