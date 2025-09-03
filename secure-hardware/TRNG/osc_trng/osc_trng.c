// main.c — CW303 (ATxmega128D4): TRUERA-style single-bit sampler (no FIFO)
// Build: make PLATFORM=CW303
// Usage from host: send "r<N>" (ASCII decimal), receive ceil(N/8) bytes tagged 'r'.
//
// Entropy mechanism: TCC0 free-runs at system clock. RTC uses independent 32 kHz RC.
// For each RTC overflow, compute delta = TCC0(CNT) - last; return (delta>>1)&1 as one random bit.

#include "hal.h"
#include "simpleserial.h"
#include <avr/io.h>
#include <avr/iox128d4.h>
#include <stdint.h>
#include <avr/interrupt.h>

/* ---------- Config ---------- */

// RTC prescaler & period -> sample rate = (32768 / prescaler) / (PER+1)
// Here: prescaler=32, PER=31  =>  (32768/32)/32 = 32 Hz  (≈32 bits/sec)
#define RTC_PRESCALE   RTC_PRESCALER_DIV32_gc
#define RTC_PER_VALUE  31

/* ---------- State ---------- */

static volatile uint16_t tcc0_last = 0;

/* ---------- Clocks & Timers ---------- */

// Free-run TCC0 @ system clock
static void tcc0_init(void) {
    TCC0.CTRLA = 0;
    TCC0.CTRLB = 0;
    TCC0.CTRLC = 0;
    TCC0.CTRLD = 0;
    TCC0.CTRLE = 0;
    TCC0.PER   = 0xFFFF; // the top value the counter counts up to before rolling over flow (returning to 0)
    TCC0.CNT   = 0;
    TCC0.CTRLA = TC_CLKSEL_DIV1_gc;   // count every CPU cycle
    tcc0_last  = TCC0.CNT;
}

// Init RTC from 32 kHz internal RC, no interrupts — we poll OVF
static void rtc_init(void) {
    // Enable 32 kHz RC
    OSC.CTRL |= OSC_RC32KEN_bm; //Turns on the internal 32 kHz RC oscillator
    while (!(OSC.STATUS & OSC_RC32KRDY_bm)) { ; } //We spin until the 32 kHz oscillator has stabilized.

    // Route 32 kHz RC to RTC and enable RTC clock
    CLK.RTCCTRL = CLK_RTCSRC_RCOSC_gc | CLK_RTCEN_bm;

    // Set prescaler and period
    RTC.CTRL = 0;                      // stop to configure safely
    RTC.PER  = RTC_PER_VALUE;          // overflow every (PER+1) ticks
    RTC.CNT  = 0;
    RTC.CTRL = RTC_PRESCALE;

    // Clear any pending flags
    RTC.INTFLAGS = RTC_OVFIF_bm;
}

/* ---------- Random bit sampler (blocking, polled) ---------- */

// Returns one random bit by waiting for the next RTC overflow
static uint8_t rng_get_bit(void) {
    // Wait for overflow
    while (!(RTC.INTFLAGS & RTC_OVFIF_bm)) { ; }
    RTC.INTFLAGS = RTC_OVFIF_bm;

    // Measure delta on fast counter
    uint16_t now   = TCC0.CNT;
    uint16_t delta = (uint16_t)(now - tcc0_last);
    tcc0_last = now;

    // Use bit1 (discard LSB) as the entropy bit
    return (uint8_t)((delta >> 1) & 0x1);
}

/* ---------- SimpleSerial handler ---------- */

// Parse ASCII decimal in data[0..len-1]; return default if none
static uint16_t parse_ascii_decimal(uint8_t *data, uint16_t len, uint16_t dflt) {
    uint16_t v = 0; uint8_t seen = 0;
    for (uint16_t i = 0; i < len; i++) {
        uint8_t c = data[i];
        if (c >= '0' && c <= '9') { v = (uint16_t)(v * 10 + (c - '0')); seen = 1; }
    }
    return seen ? v : dflt;
}

// 'r<N>' -> return N bits packed MSB-first
static uint8_t cmd_get_bits(uint8_t *data, uint16_t len) {
    uint16_t nbits = parse_ascii_decimal(data, len, 16);     // default: 16 bits
    if (nbits == 0) nbits = 1;
    if (nbits > 1024) nbits = 1024;                          // sane upper bound

    uint16_t nbytes = (uint16_t)((nbits + 7) >> 3);
    uint8_t out[128];                                        // supports up to 1024 bits
    if (nbytes > sizeof(out)) nbytes = sizeof(out), nbits = (uint16_t)(sizeof(out) * 8);

    // Pack MSB-first within each byte
    for (uint16_t i = 0; i < nbytes; i++) out[i] = 0;

    for (uint16_t i = 0; i < nbits; i++) {
        uint8_t bit = rng_get_bit();
        uint16_t bi = i >> 3;                // byte index
        uint8_t  bp = 7 - (i & 7);           // bit position (MSB-first)
        out[bi] |= (uint8_t)(bit << bp);
    }

    ss_send('r', out, nbytes);
    return 0x00;
}

/* ---------- Main ---------- */

int main(void) {
    platform_init();     // ChipWhisperer HAL
    init_uart();         // for SimpleSerial
    trigger_setup();     // not used here, but keeps default CW setup

    tcc0_init();
    rtc_init();

    simpleserial_init();               // default baud in HAL (115200)
    simpleserial_addcmd('r', 128, cmd_get_bits);

    sei(); // (no ISRs needed; safe to enable)

    while (1) {
        simpleserial_get();
    }
}
