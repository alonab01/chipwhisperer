// main.c — CW303 (ATxmega128D4): TRUERA-style RNG with SimpleSerial
// Build: make PLATFORM=CW303
// Command: host sends "r<NN>" to receive NN random bytes
//
// Entropy principle: measure jitter between system clock (counts TCC0) and
// the XMEGA 32 kHz RC-driven RTC (independent oscillator). On each RTC OVF,
// sample the fast counter and extract bits 1..8 of the delta count.

#include "hal.h"
#include "simpleserial.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
#include <string.h>

/* ---------------- RNG parameters ---------------- */

// RTC rate: we set RTC to 128 prescale on 32.768 kHz RC and PER=255,
// yielding ~ (32768 / 128) / 256 = 1 Hz. You can change PER for faster bytes.
#define RNG_TARGET_HZ 1u

// How many bytes the ISR will buffer for host reads.
#define RNG_FIFO_SIZE 64

/* ---------------- RNG state ---------------- */

static volatile uint16_t tcc0_last = 0;
static volatile uint8_t  rng_fifo[RNG_FIFO_SIZE];
static volatile uint8_t  rng_head = 0; // write index (ISR)
static volatile uint8_t  rng_tail = 0; // read index  (foreground)

static inline uint8_t fifo_count(void) {
    int16_t diff = (int16_t)rng_head - (int16_t)rng_tail;
    if (diff < 0) diff += RNG_FIFO_SIZE;
    return (uint8_t)diff;
}

static inline uint8_t fifo_push(uint8_t v) {
    uint8_t next = (uint8_t)((rng_head + 1) % RNG_FIFO_SIZE);
    if (next == rng_tail) return 0; // full
    rng_fifo[rng_head] = v;
    rng_head = next;
    return 1;
}

static inline uint8_t fifo_pop(uint8_t *out) {
    if (rng_tail == rng_head) return 0; // empty
    *out = rng_fifo[rng_tail];
    rng_tail = (uint8_t)((rng_tail + 1) % RNG_FIFO_SIZE);
    return 1;
}

/* ---------------- Clocking & timers ---------------- */

// Enable 32 kHz internal RC and route it to RTC, then set RTC overflow rate.
static void rtc_init(void) {
    // Enable 32 kHz RC oscillator
    OSC.CTRL |= OSC_RC32KEN_bm;
    while (!(OSC.STATUS & OSC_RC32KRDY_bm)) {
        // wait until 32 kHz RC ready
    }

    // Route 32 kHz RC to RTC and enable RTC clock
    // Sources: CLK_RTCSRC_RCOSC_gc = 32k RC; CLK_RTCEN_bm enables RTC clock
    CLK.RTCCTRL = CLK_RTCSRC_RCOSC_gc | CLK_RTCEN_bm;

    // Prescale RTC to 32k / 128 = 256 Hz
    RTC.CTRL = RTC_PRESCALER_DIV128_gc;

    // Overflow every 256 ticks => ~1 Hz
    RTC.PER = 255;

    // Clear and enable overflow interrupt (low level)
    RTC.INTFLAGS = RTC_OVFIF_bm;
    RTC.INTCTRL  = RTC_OVFINTLVL_LO_gc;

    // Enable low level interrupts in PMIC
    PMIC.CTRL |= PMIC_LOLVLEN_bm;
}

// Free-run TCC0 at system clock (fast counter).
static void tcc0_init(void) {
    // Stop & clear
    TCC0.CTRLA = 0;
    TCC0.CTRLB = 0;
    TCC0.CTRLC = 0;
    TCC0.CTRLD = 0;
    TCC0.CTRLE = 0;
    TCC0.PER   = 0xFFFF;  // 16-bit rollover
    TCC0.CNT   = 0;

    // Clock select: div1 -> count every CPU cycle (system clock)
    TCC0.CTRLA = TC_CLKSEL_DIV1_gc;
    tcc0_last  = TCC0.CNT;
}

/* ---------------- Entropy ISR ---------------- */

ISR(RTC_OVF_vect) {
    // Read fast counter and compute delta (handles rollover by modulo arithmetic)
    uint16_t cnt   = TCC0.CNT;
    uint16_t delta = (uint16_t)(cnt - tcc0_last);
    tcc0_last = cnt;

    // Extract bits 1..8 (discard LSB), as in TRUERA
    uint8_t rbyte = (uint8_t)((delta >> 1) & 0xFF);

    // Optional: 1-bit Von Neumann “whitener” example (disabled by default)
    // static uint8_t have_bit = 0, bit_stash = 0;
    // uint8_t b = (uint8_t)(delta & 1); // use true LSBs for VN if you like
    // if (!have_bit) { bit_stash = b; have_bit = 1; }
    // else { if (bit_stash != b) { rbyte = bit_stash; fifo_push(rbyte); } have_bit = 0; return; }

    (void)fifo_push(rbyte); // best effort; drop if FIFO full

    // Clear OVF flag
    RTC.INTFLAGS = RTC_OVFIF_bm;
}

/* ---------------- SimpleSerial command handlers ---------------- */

static uint8_t cmd_get_random(uint8_t *data, uint16_t len) {
    // data[0..len-1] contains ASCII bytes after 'r'; we treat it as decimal length if provided,
    // but SimpleSerial v2 typically passes raw length as 'len'. Easiest: if 'len'==1, use that byte as count.
    // For compatibility with standard CW usage (r<N bytes>), we interpret `len` bytes directly as count.
    // If you want decimal ASCII like "r32", parse here. We'll support both.

    uint16_t want = 0;

    if (len == 1) {
        want = data[0];
    } else {
        // parse ASCII decimal in data[0..len-1]
        for (uint16_t i = 0; i < len; i++) {
            uint8_t c = data[i];
            if (c >= '0' && c <= '9') {
                want = (uint16_t)(want * 10 + (c - '0'));
            }
        }
        if (want == 0) want = 16; // default if no digits supplied
    }

    // Cap to something reasonable to avoid super long blocking
    if (want > 64) want = 64;

    uint8_t out[64];
    uint16_t got = 0;

    // Block until we have 'want' bytes (RTC will drip them in)
    while (got < want) {
        uint8_t v;
        if (fifo_pop(&v)) {
            out[got++] = v;
        }
    }

    // Send back as one binary frame tagged with 'r'
    ss_send('r', out, (uint16_t)got);
    return 0x00;
}

/* ---------------- Main ---------------- */

int main(void) {
    platform_init();     // ChipWhisperer HAL: clocks, GPIO, etc.
    init_uart();         // UART for SimpleSerial
    trigger_setup();     // not used here, but keeps default CW behavior

    // Init RNG machinery
    tcc0_init();
    rtc_init();

    // SimpleSerial setup
    simpleserial_init();             // default baud from HAL (115200)
    simpleserial_addcmd('r', 64, cmd_get_random);

    sei(); // enable global interrupts

    while (1) {
        simpleserial_get(); // process host commands
    }
}
