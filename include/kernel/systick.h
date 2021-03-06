#ifndef __SYSTICK_H__
#define __SYSTICK_H__

#include <types.h>

#define time_after(goal, chasing)	((int)(goal)    - (int)(chasing) < 0)
#define time_before(goal, chasing)	((int)(chasing) - (int)(goal)    < 0)

#define sec_to_ticks(sec)		((sec) * sysfreq)
#define msec_to_ticks(sec)		(sec_to_ticks(sec) / 1000)
#define ticks_to_sec(ticks)		((ticks) / sysfreq)

extern unsigned int systick, sysfreq;

uint64_t get_systick64();
unsigned int get_curr_interval();

#endif /* __SYSTICK_H__ */
