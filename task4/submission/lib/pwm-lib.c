#include <pwm-lib.h>

static void update_period(pwm_breathe_t *p){
    p->period_ms = 1000u / p->freq_hz;
    if (p->period_ms == 0) p->period_ms = 1;
}

void pwm_breathe_init(pwm_breathe_t *p, uint8_t max_duty, uint16_t freq_hz, uint32_t speed_ms){
    p->max_duty = max_duty;
    p->freq_hz  = freq_hz;
    p->speed_ms = speed_ms;
    p->tick_ms  = 0;
    update_period(p);
}

uint8_t pwm_breathe_set_duty(pwm_breathe_t *p, int duty){
    if (duty < 0 || duty > 100) return 0;
    p->max_duty = (uint8_t)duty;
    return 1;
}

uint8_t pwm_breathe_set_freq(pwm_breathe_t *p, int freq_hz){
    if (freq_hz < 1 || freq_hz > 500) return 0;
    p->freq_hz = (uint16_t)freq_hz;
    update_period(p);
    return 1;
}

uint8_t pwm_breathe_set_speed(pwm_breathe_t *p, int speed_ms){
    if (speed_ms < 100 || speed_ms > 60000) return 0;
    p->speed_ms = (uint32_t)speed_ms;
    p->tick_ms %= p->speed_ms * 2u;
    return 1;
}

// Returns 1 if the output should be on for this 1 ms tick, then advances time.
uint8_t pwm_breathe_tick(pwm_breathe_t *p){
    uint32_t cycle = p->speed_ms * 2u;
    uint32_t phase = p->tick_ms % cycle;
    uint32_t level = phase <= p->speed_ms
        ? (phase * 100u) / p->speed_ms
        : ((cycle - phase) * 100u) / p->speed_ms;
    uint32_t duty = (level * p->max_duty) / 100u;
    uint32_t on_time = (p->period_ms * duty) / 100u;
    uint8_t on = (p->tick_ms % p->period_ms) < on_time;
    p->tick_ms++;
    return on;
}
