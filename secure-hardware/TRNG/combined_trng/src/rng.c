#include "rng.h"
#include "adc_utils.h"
#include "simpleserial.h"
#include <util/delay.h>
#include <avr/io.h>


static uint16_t bitbuf = 0;     // buffer for leftover bits
static uint8_t bits_in_buf = 0; // how many bits are currently in buffer

// --- RTC/TCC0 counter jitter-based source ---
uint8_t rng_get_byte_counter(void) {
    while (!(RTC.INTFLAGS & RTC_OVFIF_bm)) { ; }
    RTC.INTFLAGS = RTC_OVFIF_bm;
    uint8_t counter_value = TCC0.CNTL; 
    TCC0.CNT = 0;
    return counter_value;
}



// --- Mixed VCC + TEMP source with buffering ---
uint8_t rng_get_byte_vcc_temp(void) {
    while (bits_in_buf < 8) {
        uint16_t vcc  = sample_adc(ADC_CH_MUXINT_SCALEDVCC_gc, 0);
        uint16_t temp = sample_adc(ADC_CH_MUXINT_TEMP_gc, 0);

        // Take 3 bits from VCC, 3 bits from TEMP = 6 fresh bits
        uint8_t newbits = (uint8_t)((vcc & 0x07) << 3) | (temp & 0x07);

        // Push into bitbuf
        bitbuf |= ((uint16_t)newbits << bits_in_buf);
        bits_in_buf += 6;
    }

    // Extract 8 bits
    uint8_t out = (uint8_t)(bitbuf & 0xFF);
    bitbuf >>= 8;
    bits_in_buf -= 8;

    return out;
}
void crc_feed_byte(uint8_t b) {
    // Wait until not busy
    while (CRC.STATUS & CRC_BUSY_bm);
    CRC.DATAIN = b;
}

uint8_t rng_get_crc_byte(void) {

    for (uint8_t i = 0; i < CRC_BYTES; i++) {
        uint8_t rnd = rng_get_byte_vcc_temp();
        crc_feed_byte(rnd);
    }
    // Wait until finished
    while (CRC.STATUS & CRC_BUSY_bm);

    // Fold all 4 bytes → 1
    return CRC.CHECKSUM0 ^ CRC.CHECKSUM1 ^ CRC.CHECKSUM2 ^ CRC.CHECKSUM3;
}


// --- Dispatcher: choose RNG source based on scmd ---
uint8_t get_random_bytes(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data) {
    uint8_t N = data[0];
    static uint8_t out[CHUNK_SIZE];

    for (uint16_t i = 0; i < N; i++) {
        switch (scmd) {
            case 0: out[i] = rng_get_byte_counter(); break;
            case 1: out[i] = rng_get_byte_vcc_temp(); break;
            case 2: out[i] = rng_get_crc_byte(); break;
            default: out[i] = 0xFF; break;
        }
    }

    simpleserial_put('r', N, out);
    return 0;
}
