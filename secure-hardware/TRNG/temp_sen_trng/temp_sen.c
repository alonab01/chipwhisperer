// main.c — CW303 (ATxmega128D4): read internal temperature sensor via ADCA
// Build: make PLATFORM=CW303
//
// Host usage (SimpleSerial):
//   send 't' (no payload) -> device replies with 1 byte: 8 LSBs of the temp-sensor ADC reading.

#include "hal.h"
#include "simpleserial.h"
// #include <avr/io.h>
#include <stdint.h>
// #include <avr/iox128d4.h>


// ---------- ADC: init for internal temperature sensor ----------
static void adc_init_temp(void) {
    // Reference: internal 1.0V; enable temp sensor & bandgap path
    ADCA.REFCTRL   = ADC_REFSEL_INT1V_gc | ADC_TEMPREF_bm ;

    // 12-bit resolution (unsigned). Keep ADC clock ≈ 125 kHz for internal channels:
    // 32 MHz / 256 = 125 kHz
    ADCA.CTRLB     = ADC_RESOLUTION_12BIT_gc;
    ADCA.PRESCALER = ADC_PRESCALER_DIV256_gc;

    // Channel 0: internal input mode, 1x gain
    ADCA.CH0.CTRL    = ADC_CH_INPUTMODE_INTERNAL_gc | ADC_CH_GAIN_1X_gc;

    // Route CH0 positive MUX to internal temperature sensor
    ADCA.CH0.MUXCTRL = ADC_CH_MUXINT_TEMP_gc;

    // Enable ADC
    ADCA.CTRLA = ADC_ENABLE_bm;

    // One dummy conversion after switching internal source (recommended)
    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while (!(ADCA.INTFLAGS & ADC_CH0IF_bm)) {;}
    ADCA.INTFLAGS = ADC_CH0IF_bm;
}

// Single 12-bit conversion from the temp sensor (polling)
static  uint16_t adc_read_temp_u12(void) {
    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while (!(ADCA.INTFLAGS & ADC_CH0IF_bm)) {;}
    ADCA.INTFLAGS = ADC_CH0IF_bm;    // clear flag
    return ADCA.CH0.RES;             // 12-bit unsigned result in low bits
}

// ---------- SimpleSerial callback: return 8 LSBs of temp reading ----------
static uint8_t cmd_get_temp_bits(uint8_t* data, uint8_t len) {
    (void)data; (void)len;
    uint16_t r  = adc_read_temp_u12();
    uint8_t  random_byte = (uint8_t)(r & 0xFF);
    simpleserial_put('x',1, &random_byte);
    return 0;
}

int main(void) {
    platform_init();
    init_uart();
    trigger_setup();   // not used here, but harmless for CW projects

    adc_init_temp();

    simpleserial_init();
    simpleserial_addcmd('t', 0, cmd_get_temp_bits);

    while (1) {
        simpleserial_get();
    }
}
