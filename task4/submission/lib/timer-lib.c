#include <timer-lib.h>

// TIM2 free-running 16-bit counter at 1 kHz (1 ms/tick).
void timer_ms_init(void){
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_TimeBaseInitTypeDef t = {0};
    t.TIM_Prescaler     = (uint16_t)((SystemCoreClock / 1000u) - 1u);
    t.TIM_CounterMode   = TIM_CounterMode_Up;
    t.TIM_Period        = 0xFFFF;
    t.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &t);
    TIM_Cmd(TIM2, ENABLE);
}
uint16_t timer_ms(void){ return (uint16_t)TIM2->CNT; }

