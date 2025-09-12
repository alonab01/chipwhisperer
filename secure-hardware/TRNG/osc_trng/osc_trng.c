#include <stdint.h>
#include "hal.h"
#include "simpleserial.h"
// #include <avr/iox128d4.h>

#define RTC_PER_VALUE  20000  
#define CHUNK_SIZE 249 // max chunk size for simpleserial2


// ---------- Forward Declarations ----------
static void rtc_init(void);
static void tcc0_init(void);

static uint8_t rng_get_byte(void);
static uint8_t get_random_bytes(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data);
static uint8_t get_random_bytes_not_limted(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data);
static uint8_t debug(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data);



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
    TCC0.PER   = 0x00FF; // max period (rollover at 65535)
    TCC0.CNT   = 0;      // reset counter
    TCC0.CTRLA = TC_CLKSEL_DIV1_gc; // run from system clock (32 MHz)
}

// Returns one random bit by waiting for the next RTC overflow
static uint8_t rng_get_byte(void) {
    // Wait for overflow
    while (!(RTC.INTFLAGS & RTC_OVFIF_bm)) { ; }
    RTC.INTFLAGS = RTC_OVFIF_bm;
    uint8_t counter_value = TCC0.CNTL; 
    TCC0.CNT = 0; // reset TCC0 counter
    return counter_value; // read low byte of TCC0 counter
}

static uint8_t debug(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data) {
    uint8_t b[3] = {0xa5, 0x5a, 0xc0};
    simpleserial_put('r', 3, b);             // NOTE: header 'z', then &buf, len
    return 0;
}

static uint8_t get_random_bytes(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data) {
    // --- Decide how many bytes we need to interpret ---
    uint8_t N = 0;
    N = (uint8_t)data[0];              // 0–255

    static uint8_t out[CHUNK_SIZE];
    uint32_t sent = 0;
    // Fill this chunk
    for (uint16_t i = 0; i < N; i += 1) {
        out[i] = rng_get_byte();
    }
    simpleserial_put('r', N, out);
    return 0;
}


static uint8_t get_random_bytes_not_limted(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data) {
    // --- Decide how many bytes we need to interpret ---
    uint32_t N = 0;

    if (dlen == 1) {
        N = (uint8_t)data[0];              // 0–255
    } else if (dlen == 2) {
        N = ((uint16_t)data[1] << 8) | data[0];  // 0–65535
    } else if (dlen == 3) {
        N = ((uint32_t)data[2] << 16) |
            ((uint32_t)data[1] << 8)  |
            (uint32_t)data[0];             // 0–16M
    } else if (dlen == 4) {
        N = ((uint32_t)data[3] << 24) |
            ((uint32_t)data[2] << 16) |
            ((uint32_t)data[1] << 8)  |
            (uint32_t)data[0];             // 0–4.29B
    } else {
        return 1;   // invalid
    }
    uint8_t out[CHUNK_SIZE];
    uint32_t sent = 0;

    while (sent < N) {
        uint16_t chunk = (N - sent > CHUNK_SIZE) ? CHUNK_SIZE : (N - sent);

        // Fill this chunk
        for (uint16_t i = 0; i < chunk; i += 1) {
            out[i] = rng_get_byte();
        }
        simpleserial_put('r', chunk, out);
        sent += chunk;
    }

    return 0;
}


/* ---------- Main ---------- */
int main(void) {
    platform_init();
    init_uart();
    trigger_setup();

    rtc_init();
    tcc0_init();
    simpleserial_init();

    simpleserial_addcmd('b', 1, get_random_bytes);
    simpleserial_addcmd('l', 0, get_random_bytes_not_limted);
    simpleserial_addcmd('c', 0, debug );


    while (1) {
        simpleserial_get();
    }
}
