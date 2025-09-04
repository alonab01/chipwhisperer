// #include <stdint.h>
// #include "hal.h"
// #include "simpleserial.h"


// #define RTC_PRESCALE   RTC_PRESCALER_DIV32_gc
// #define RTC_PER_VALUE  31

// /* ---------- State ---------- */

// static volatile uint16_t tcc0_last = 0;

// /* ---------- Clocks & Timers ---------- */

// // Free-run TCC0 @ system clock
// static void tcc0_init(void) {
//     TCC0.CTRLA = 0;
//     TCC0.CTRLB = 0;
//     TCC0.CTRLC = 0;
//     TCC0.CTRLD = 0;
//     TCC0.CTRLE = 0;
//     TCC0.PER   = 0xFFFF; // the top value the counter counts up to before rolling over flow (returning to 0)
//     TCC0.CNT   = 0;
//     TCC0.CTRLA = TC_CLKSEL_DIV1_gc;   // count every CPU cycle
//     tcc0_last  = TCC0.CNT;
// }

// // Init RTC from 32 kHz internal RC, no interrupts — we poll OVF
// static void rtc_init(void) {
//     // Enable 32 kHz RC
//     OSC.CTRL |= OSC_RC32KEN_bm; //Turns on the internal 32 kHz RC oscillator
//     while (!(OSC.STATUS & OSC_RC32KRDY_bm)) { ; } //We spin until the 32 kHz oscillator has stabilized.

//     // Route 32 kHz RC to RTC and enable RTC clock
//     CLK.RTCCTRL = CLK_RTCSRC_RCOSC_gc | CLK_RTCEN_bm;

//     // Set prescaler and period
//     RTC.CTRL = 0;                      // stop to configure safely
//     RTC.PER  = RTC_PER_VALUE;          // overflow every (PER+1) ticks
//     RTC.CNT  = 0;
//     RTC.CTRL = RTC_PRESCALE;                   //RTC_PRESCALE

//     // Clear any pending flags
//     RTC.INTFLAGS = RTC_OVFIF_bm;
// }

// /* ---------- Random bit sampler (blocking, polled) ---------- */

// // Returns one random bit by waiting for the next RTC overflow
// static uint8_t rng_get_bit(void) {
//     // Wait for overflow
//     while (!(RTC.INTFLAGS & RTC_OVFIF_bm)) { ; }
//     RTC.INTFLAGS = RTC_OVFIF_bm;

//     // Measure delta on fast counter
//     uint16_t now   = TCC0.CNT;
//     uint16_t delta = (uint16_t)(now - tcc0_last);
//     tcc0_last = now;

//     // Use bit1 (discard LSB) as the entropy bit
//     return (uint8_t)((delta >> 1) & 0x1);
// }

// static uint8_t cmd_get_bits(uint8_t *data, uint8_t len) {
//     if (len < 1) return 0x00;      // safety check

//     uint16_t nbits = data[0];      // first byte = number of bits requested
//     if (nbits == 0) nbits = 1;     // at least 1 bit

//     uint16_t nbytes = (nbits + 7) >> 3;  // ceiling(nbits/8)
//     uint8_t out[32] = {0};               // 32 bytes = 256 bits max

//     // Pack bits MSB-first into each byte
//     for (uint16_t i = 0; i < nbits; i++) {
//         // uint8_t bit = rng_get_bit();
//         uint8_t bit = 1;
//         uint16_t bi = i >> 3;        // byte index
//         uint8_t  bp = 7 - (i & 7);   // bit position (MSB-first)
//         out[bi] |= (uint8_t)(bit << bp);
//     }

//     simpleserial_put('r', nbytes, out);
//     return 0x00;


// }
// int main(void) {
//     platform_init();     // ChipWhisperer HAL
//     init_uart();         // for SimpleSerial
//     trigger_setup();     // not used here, but keeps default CW setup


//     tcc0_init();
//     rtc_init();

//     simpleserial_init();               // default baud in HAL (115200)
//     simpleserial_addcmd('o', 1, cmd_get_bits);


//     while (1) {
//         simpleserial_get();
//     }
// }

#include <stdint.h>
#include "hal.h"
#include "simpleserial.h"
// #include <avr/iox128d4.h>

#define RTC_PER_VALUE  32767   


static void rtc_init(void) {
    OSC.CTRL |= OSC_RC32KEN_bm;            // enable 32k oscillator
    while (!(OSC.STATUS & OSC_RC32KRDY_bm));

    CLK.RTCCTRL = CLK_RTCSRC_RCOSC32_gc | CLK_RTCEN_bm;

    RTC.PER  = RTC_PER_VALUE;                               // set top
    RTC.CNT  = 0;                                           // reset counter
    RTC.CTRL = RTC_PRESCALER_DIV1_gc;                       // enable and start
    RTC.INTFLAGS = RTC_OVFIF_bm | RTC_COMPIF_bm;            // clear flags
}


static void tcc0_init(void) {
    TCC0.CTRLA = 0;      // stop timer during setup
    TCC0.PER   = 0xFFFF; // max period (rollover at 65535)
    TCC0.CNT   = 0;      // reset counter
    TCC0.CTRLA = TC_CLKSEL_DIV1_gc; // run from system clock (32 MHz)
}


/* ---------- SimpleSerial handler ---------- */
static uint8_t cmd_read_cnt(uint8_t *data, uint8_t len) {
    uint16_t cnt16 = RTC.CNT;
    simpleserial_put('r', 2, (uint8_t *)&cnt16);
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
    simpleserial_addcmd('o', 1, cmd_read_cnt);
    simpleserial_addcmd('u', 0, cmd_debug);

    while (1) {
        simpleserial_get();
    }
}
