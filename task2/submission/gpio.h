#ifndef GPIO_H
#define GPIO_H

#include <ch32v00x.h>

// Pin under test: silkscreen "PC6" on the VSDSquadron Mini header = CH32V003 GPIO port C, pin 6.
#define LED_GPIO_RCC   RCC_APB2Periph_GPIOC
#define LED_GPIO_PORT  GPIOC
#define LED_GPIO_PIN   GPIO_Pin_6
#define LED_GPIO_NAME  "PC6"

void    gpio_init(void);
void    gpio_set(void);
void    gpio_clear(void);
void    gpio_toggle(void);
uint8_t gpio_get(void);

#endif // GPIO_H
