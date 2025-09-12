
#include "hal.h"
#include "simpleserial.h"
#include <stdint.h>
#include <avr/iox128d4.h>


#define CHUNK_SIZE 249 // max chunk size for simpleserial2


static inline void adc_clear_existing_vars(void);
static inline uint16_t sample_temp_sens(void);
static void adc_init_internal(void);
static uint8_t rng_get_byte(void);
static uint8_t get_random_bytes(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data);



// ---------- Bit-packer state ----------
static uint16_t bitbuf = 0;     // holds leftover bits between samples
static uint8_t bits_in_buf = 0;


// --- one-shot conversion helper on CH0 (with 1 dummy after MUX switch) ---
static inline void adc_clear_existing_vars(void) {
    // dummy conversion to settle
    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while (!(ADCA.INTFLAGS & ADC_CH0IF_bm)) {}
    (void)ADCA.CH0.RES;                   // read -> clears flag, discard

}


static inline uint16_t sample_temp_sens(void) {
    // real conversion
    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while (!(ADCA.INTFLAGS & ADC_CH0IF_bm)) {}
    return ADCA.CH0.RES;                  // read -> clears flag, keep
}

// ---------- ADC init for internal sources on CH0 ----------
static void adc_init_internal(void) {
    ADCA.CTRLA = 0; // ensure disabled during config

    ADCA.PRESCALER = ADC_PRESCALER_DIV256_gc;                 // ~125 kHz @ 32 MHz CPU
    ADCA.REFCTRL   = ADC_REFSEL_INT1V_gc | ADC_TEMPREF_bm;    // 1.00V ref, enable temp path
    ADCA.CTRLB     = ADC_RESOLUTION_12BIT_gc;                 // unsigned, 12-bit

    ADCA.CH0.CTRL  = ADC_CH_INPUTMODE_INTERNAL_gc | ADC_CH_GAIN_1X_gc;

    ADCA.CTRLA     = ADC_ENABLE_bm;                           // enable last

    ADCA.CH0.MUXCTRL = ADC_CH_MUXINT_TEMP_gc;

    adc_clear_existing_vars(); // clear out any existing variables
}



// Pack 5-bit ADC outputs into full bytes
static uint8_t rng_get_byte(void) {
    while (bits_in_buf < 8) {
        uint8_t five = rng_get_bits() & 0x1F;   // 5 LSBs from ADC
        bitbuf |= ((uint32_t)five << bits_in_buf);
        bits_in_buf += 5;
    }

    uint8_t out = (uint8_t)(bitbuf & 0xFF);
    bitbuf >>= 8;
    bits_in_buf -= 8;
    return out;
}

static uint8_t get_random_bytes(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data) {
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


int main(void) {
    platform_init();
    init_uart();
    trigger_setup();

    adc_init_internal();

    simpleserial_init();
    simpleserial_addcmd('b', 0, get_random_bytes);

    while (1) {
        simpleserial_get();
    }
}

