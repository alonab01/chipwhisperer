#ifndef INIT_H
#define INIT_H

#include <stdint.h>

#define RTC_PER_VALUE 10000

void rtc_init(void);
void tcc0_init(void);
void crc_init(void);

#endif
