#ifndef __PWM_LIB_SRC_PWM_LIB_H
#define __PWM_LIB_SRC_PWM_LIB_H

#include <stdint.h>

// Software PWM with a triangular breathing envelope; call pwm_breathe_tick() once per 1 ms.
typedef struct {
    uint8_t  max_duty;   // 0..100 %
    uint16_t freq_hz;    // 1..500
    uint32_t period_ms;
    uint32_t speed_ms;   // ramp time off->max; a full breath takes 2x
    uint32_t tick_ms;
} pwm_breathe_t;

void    pwm_breathe_init(pwm_breathe_t *p, uint8_t max_duty, uint16_t freq_hz, uint32_t speed_ms);
uint8_t pwm_breathe_set_duty(pwm_breathe_t *p, int duty);
uint8_t pwm_breathe_set_freq(pwm_breathe_t *p, int freq_hz);
uint8_t pwm_breathe_set_speed(pwm_breathe_t *p, int speed_ms);
uint8_t pwm_breathe_tick(pwm_breathe_t *p);

#endif // __PWM_LIB_SRC_PWM_LIB_H
