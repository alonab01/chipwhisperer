#include "rng.h"
#include "timers.h"
#include "adc_utils.h"
#include "simpleserial.h"
#include <util/delay.h>

uint8_t rng_get_byte_counter(void) {
    while (!(RTC.INTFLAGS & RTC_OVFIF_bm)) { ; }
    RTC.INTFLAGS = RTC_OVFIF_bm;
    uint8_t counter_value = TCC0.CNTL; 
    TCC0.CNT = 0;
    return counter_value;
}

uint8_t rng_get_byte_temp_sens(void) {
    uint8_t four1 = (uint8_t)(sample_temp_sens(ADC_CH_MUXINT_TEMP_gc) & 0x0F);
    _delay_ms(10);
    uint8_t four2 = (uint8_t)(sample_temp_sens(ADC_CH_MUXINT_TEMP_gc) & 0x0F);
    return (four1 << 4) | four2;
}

uint8_t rng_get_byte_vcc3_temp5(void) {
    adc_clear_existing_vars(ADC_CH_MUXINT_SCALEDVCC_gc);
    uint16_t vcc  = sample_temp_sens(ADC_CH_MUXINT_SCALEDVCC_gc);
    adc_clear_existing_vars(ADC_CH_MUXINT_TEMP_gc);
    uint16_t temp = sample_temp_sens(ADC_CH_MUXINT_TEMP_gc);

    uint8_t top3 = (uint8_t)(vcc  & 0x07);
    uint8_t low5 = (uint8_t)(temp & 0x1F);
    return (uint8_t)((top3 << 5) | low5);
}

uint8_t get_random_bytes(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data) {
    uint8_t N = data[0];
    static uint8_t out[CHUNK_SIZE];

    for (uint16_t i = 0; i < N; i++) {
        switch (scmd) {
            case 0: out[i] = rng_get_byte_counter(); break;
            case 1: out[i] = rng_get_byte_temp_sens(); break;
            case 2: out[i] = rng_get_byte_vcc3_temp5(); break;
            default: out[i] = 0xFF; break;
        }
    }

    simpleserial_put('r', N, out);
    return 0;
}
