#include "init.h"
#include <avr/io.h>



void rtc_init(void) {
    OSC.CTRL |= OSC_RC32KEN_bm;              
    while (!(OSC.STATUS & OSC_RC32KRDY_bm));

    CLK.RTCCTRL = CLK_RTCSRC_RCOSC32_gc | CLK_RTCEN_bm;

    while (RTC.STATUS & RTC_SYNCBUSY_bm);
    RTC.PER = RTC_PER_VALUE;

    while (RTC.STATUS & RTC_SYNCBUSY_bm);
    RTC.CNT = 0;

    while (RTC.STATUS & RTC_SYNCBUSY_bm);
    RTC.CTRL = RTC_PRESCALER_DIV1_gc;

    while (RTC.STATUS & RTC_SYNCBUSY_bm);
    RTC.INTFLAGS = RTC_OVFIF_bm | RTC_COMPIF_bm;
}

void tcc0_init(void) {
    TCC0.CTRLA = 0;
    TCC0.PER   = 0x00FF;
    TCC0.CNT   = 0;
    TCC0.CTRLA = TC_CLKSEL_DIV1_gc;
}

void crc_init(void) {
    CRC.CTRL = CRC_CRC32_bm | CRC_SOURCE_IO_gc |CRC_RESET_RESET1_gc;
    // Wait until finished
    while (CRC.STATUS & CRC_BUSY_bm);
}
