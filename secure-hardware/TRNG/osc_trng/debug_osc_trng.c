#include <stdint.h>
#include "hal.h"
#include "simpleserial.h"
// #include <avr/iox128d4.h>

#define RTC_PER_VALUE  32767  // overflow every(1 second with DIV1 prescaler)
// #define RTC_PER_VALUE  31  // overflow every 32 ticks (1 second with DIV1 prescaler)


// ---------- Forward Declarations ----------
static void rtc_init(void);
static void tcc0_init(void);

static uint8_t cmd_read_cnt_rtc(uint8_t *data, uint8_t len);
static uint8_t cmd_read_cnt_tcc0(uint8_t *data, uint8_t len);
static uint16_t rng_get_bits(void);
static uint8_t cmd_get_bits(uint8_t *data, uint8_t len);
static uint8_t cmd_debug(uint8_t *data, uint8_t len);





static void rtc_init(void) {
    // 1. Enable 32 kHz oscillator
    OSC.CTRL |= OSC_RC32KEN_bm;              
    while (!(OSC.STATUS & OSC_RC32KRDY_bm)); // wait until oscillator is stable

    // 2. Route oscillator to RTC
    CLK.RTCCTRL = CLK_RTCSRC_RCOSC32_gc | CLK_RTCEN_bm;

    // 3. Configure RTC registers (always wait for SYNCBUSY between writes)

    while (RTC.STATUS & RTC_SYNCBUSY_bm);    // wait before writing PER
    RTC.PER = RTC_PER_VALUE;                 // set top value

    while (RTC.STATUS & RTC_SYNCBUSY_bm);    // wait before writing CNT
    RTC.CNT = 0;                             // reset counter

    while (RTC.STATUS & RTC_SYNCBUSY_bm);    // wait before writing CTRL
    RTC.CTRL = RTC_PRESCALER_DIV1_gc;        // enable and start

    while (RTC.STATUS & RTC_SYNCBUSY_bm);    // wait before clearing flags
    RTC.INTFLAGS = RTC_OVFIF_bm | RTC_COMPIF_bm; // clear overflow & compare flags
}



static void tcc0_init(void) {
    TCC0.CTRLA = 0;      // stop timer during setup
    TCC0.PER   = 0xFFFF; // max period (rollover at 65535)
    TCC0.CNT   = 0;      // reset counter
    TCC0.CTRLA = TC_CLKSEL_DIV1_gc; // run from system clock (32 MHz)
}


/* ---------- SimpleSerial handler ---------- */
static uint8_t cmd_read_cnt_rtc(uint8_t *data, uint8_t len) {
    uint16_t cnt16 = RTC.CNT;
    simpleserial_put('x', 2, (uint8_t *)&cnt16);
    return 0x00;
}

static uint8_t cmd_read_cnt_tcc0(uint8_t *data, uint8_t len) {
    uint16_t cnt16 = TCC0.CNT;
    // uint16_t cnt16 = 0x1234;
    simpleserial_put('x', 2, (uint8_t *)&cnt16);
    return 0x00;
} 

// Returns one random bit by waiting for the next RTC overflow
static uint16_t rng_get_bits(void) {
    // Wait for overflow
    while (!(RTC.INTFLAGS & RTC_OVFIF_bm)) { ; }
    RTC.INTFLAGS = RTC_OVFIF_bm;
    return TCC0.CNTL,Tcc; // read low byte of TCC0 counter
}


static uint8_t cmd_get_bits(uint8_t *data, uint8_t len) {

    uint8_t out = rng_get_bits();
    // uint16_t oust = 0x1234;
    simpleserial_put('x', 1, &out);
    return 0x00;

}


static uint8_t cmd_debug(uint8_t *data, uint8_t len) {
    uint8_t regs[4];
    regs[0] = CLK.RTCCTRL;
    regs[1] = RTC.CTRL;
    regs[2] = RTC.INTFLAGS;
    regs[3] = OSC.STATUS;
    simpleserial_put('d', 4, regs);
    return 0x00;
}






/* ---------- Main ---------- */
int main(void) {
    platform_init();
    init_uart();
    trigger_setup();

    rtc_init();
    tcc0_init();
    simpleserial_init();

    // Register command 'o' to read counter
    simpleserial_addcmd('o', 0, cmd_read_cnt_rtc);
    simpleserial_addcmd('x', 0, cmd_read_cnt_tcc0);
    simpleserial_addcmd('u', 0, cmd_debug);
    simpleserial_addcmd('a', 0, cmd_get_bits);

    while (1) {
        simpleserial_get();
    }
}
