#ifndef TIMERS_H
#define TIMERS_H

#include <avr/io.h>
#include <stdint.h>

#define RTC_PER_VALUE 20000

void rtc_init(void);
void tcc0_init(void);

#endif
