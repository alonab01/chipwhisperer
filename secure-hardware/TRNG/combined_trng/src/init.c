#include "init.h"
#include <avr/io.h>

void rtc_init(void)
{
    // Enable 32kHz internal oscillator for RTC
    OSC.CTRL |= OSC_RC32KEN_bm;

    // Wait for the oscillator to stabilize
    while (!(OSC.STATUS & OSC_RC32KRDY_bm))
        ;

    // Configure RTC to use the 32kHz oscillator and enable it
    CLK.RTCCTRL = CLK_RTCSRC_RCOSC32_gc | CLK_RTCEN_bm;

    // Make sure RTC is not busy before configuration
    while (RTC.STATUS & RTC_SYNCBUSY_bm)
        ;

    // Configure RTC period
    RTC.PER = RTC_PER_VALUE;

    // Make sure RTC is not busy before configuration
    while (RTC.STATUS & RTC_SYNCBUSY_bm)
        ;

    // Reset the RTC counter
    RTC.CNT = 0;

    // Make sure RTC is not busy before configuration
    while (RTC.STATUS & RTC_SYNCBUSY_bm)
        ;

    // Set RTC prescaler to 1 (no prescaling)
    RTC.CTRL = RTC_PRESCALER_DIV1_gc;

    // Make sure RTC is not busy before enabling interrupts
    while (RTC.STATUS & RTC_SYNCBUSY_bm)
        ;

    // Clear any existing interrupt flags
    RTC.INTFLAGS = RTC_OVFIF_bm | RTC_COMPIF_bm;
}

void tcc0_init(void)
{
    // Configure TCC0 settings
    // TCC0 clock is clk_per which by default is the 2 MHz internal oscillator
    OSC.CTRL |= OSC_RC32MEN_bm;                  // enable 32 MHz oscillator
    while (!(OSC.STATUS & OSC_RC32MRDY_bm));     // wait until ready
    CLK.CTRL = CLK_SCLKSEL_RC32M_gc; // Set system clock to 32 MHz
    TCC0.CTRLA = 0;
    TCC0.PER = 0x10FF;
    TCC0.CNT = 0;
    TCC0.CTRLA = TC_CLKSEL_DIV1_gc;
}

void crc_init(void)
{
    // Enable the CRC module and select CRC-32 mode with input from I/O (software)
    CRC.CTRL = CRC_RESET_RESET1_gc;
    CRC.CTRL = CRC_CRC32_bm | CRC_SOURCE_IO_gc;
}
