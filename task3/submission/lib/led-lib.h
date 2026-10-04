#ifndef __LED_LIB_SRC_LED_LIB_H
#define __LED_LIB_SRC_LED_LIB_H

#include <stdint.h>

// LED on PC6, push-pull, active-high.
void    led_init(void);
void    led_on(void);
void    led_off(void);
void    led_toggle(void);
uint8_t led_get(void);

#endif // __LED_LIB_SRC_LED_LIB_H
