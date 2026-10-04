#include "gpio.h"

void gpio_init(void){
    RCC_APB2PeriphClockCmd(LED_GPIO_RCC, ENABLE);

    GPIO_InitTypeDef gpio = {0};
    gpio.GPIO_Pin   = LED_GPIO_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_GPIO_PORT, &gpio);

    gpio_clear();
}

void gpio_set(void){
    GPIO_WriteBit(LED_GPIO_PORT, LED_GPIO_PIN, Bit_SET);
}

void gpio_clear(void){
    GPIO_WriteBit(LED_GPIO_PORT, LED_GPIO_PIN, Bit_RESET);
}

void gpio_toggle(void){
    if (gpio_get()) gpio_clear(); else gpio_set();
}

// Returns the level currently driven on the pin (1 = HIGH, 0 = LOW).
uint8_t gpio_get(void){
    return GPIO_ReadOutputDataBit(LED_GPIO_PORT, LED_GPIO_PIN);
}
