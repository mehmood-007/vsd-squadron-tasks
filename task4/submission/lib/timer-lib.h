#ifndef __TIMER_LIB_SRC_TIMER_LIB_H
#define __TIMER_LIB_SRC_TIMER_LIB_H

#include <ch32v00x.h>
#include <debug.h>

void timer_ms_init(void);
uint16_t timer_ms(void);

#endif // __TIMER_LIB_SRC_TIMER_LIB_H
