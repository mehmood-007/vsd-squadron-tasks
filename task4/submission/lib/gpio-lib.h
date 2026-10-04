#ifndef __GPIO_LIB_SRC_GPIO_LIB_H
#define __GPIO_LIB_SRC_GPIO_LIB_H

#include <ch32v00x.h>
#include <debug.h>

#define GPIO_PORT_BLINK GPIOC
#define GPIO_PIN_BLINK GPIO_Pin_6

// Button on PC7. 1 = active-high (pressed reads 1), 0 = active-low (pressed reads 0).
#define BTN_ACTIVE_HIGH 0
#define BTN_PORT GPIOC
#define BTN_PORT_RCC RCC_APB2Periph_GPIOC
#define BTN_PIN  GPIO_Pin_7
#define BTN_DEBOUNCE_MS 20

void gpio_init(void);
void gpio_set(uint8_t ledState);
void gpio_clear(void);
void toggle_blink_led(void);

void    button_init(void);
uint8_t button_pressed(void);
uint8_t button_pressed_debounced(void);
void    button_wait_release(void);

// Reads a pin named like "C6"/"d3"/"A2" as floating input. Returns 0/1, or -1 if the name is invalid.
int8_t  gpio_read_pin(const char *pin);

#endif // __GPIO_LIB_SRC_GPIO_LIB_H