#ifndef __TIMER_H__
#define __TIMER_H__

#include "types.h"
#define CLOCK_FREQ 10000000
#define TICKS_PER_SEC 1000

void set_next_trigger();
void timer_init();
uint64_t get_time_us();
void sys_set_trigger();

#endif