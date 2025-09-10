// main.c — CW303 (ATxmega128D4)
// Returns one byte per 't':
//   [7:5] = 3 LSBs of VCC/10, [4:0] = 5 LSBs of TEMP (internal temp sensor)
//
// Build: make PLATFORM=CW303
// Host:  target.simpleserial_write('t', b''); resp = target.simpleserial_read('t', 1)

#include "hal.h"
#include "simpleserial.h"
#include <stdint.h>
// #include <avr/iox128d4.h>




// --- one-shot conversion helper on CH0 (with 1 dummy after MUX switch) ---
static inline uint16_t adc_conv_ch0_after_mux(uint8_t muxsel) {
    ADCA.CH0.MUXCTRL = muxsel;
    // dummy conversion to settle
    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while (!(ADCA.INTFLAGS & ADC_CH0IF_bm)) {}
    (void)ADCA.CH0.RES;                   // read -> clears flag, discard
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
}

// ---------- Make one byte: [7:5]=VCC/10 LSB3, [4:0]=TEMP LSB5 ----------
static uint8_t make_byte_vcc3_temp5(void) {
    uint16_t vcc  = adc_conv_ch0_after_mux(ADC_CH_MUXINT_SCALEDVCC_gc);
    uint16_t temp = adc_conv_ch0_after_mux(ADC_CH_MUXINT_TEMP_gc);

    uint8_t top3 = (uint8_t)(vcc  & 0x07);    // 3 LSBs
    uint8_t low5 = (uint8_t)(temp & 0x1F);    // 5 LSBs
    return (uint8_t)((top3 << 5) | low5);
}

// ---------- SimpleSerial callback ----------
static uint8_t cmd_get_mixed(uint8_t *data, uint8_t len) {
    (void)data; (void)len;
    uint8_t b = make_byte_vcc3_temp5();
    simpleserial_put('t', 1, &b);             // NOTE: header 't', then &buf, len
    return 0;
}

int main(void) {
    platform_init();
    init_uart();
    trigger_setup();

    adc_init_internal();

    simpleserial_init();
    simpleserial_addcmd('t', 0, cmd_get_mixed);

    while (1) {
        simpleserial_get();
    }
}
