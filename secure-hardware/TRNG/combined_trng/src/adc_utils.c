#include "adc_utils.h"

void adc_init_internal(void) {
    ADCA.CTRLA = 0;

    ADCA.PRESCALER = ADC_PRESCALER_DIV256_gc;
    ADCA.REFCTRL   = ADC_REFSEL_INT1V_gc | ADC_TEMPREF_bm;
    ADCA.CTRLB     = ADC_RESOLUTION_12BIT_gc;

    ADCA.CH0.CTRL  = ADC_CH_INPUTMODE_INTERNAL_gc | ADC_CH_GAIN_1X_gc;

    ADCA.CTRLA     = ADC_ENABLE_bm;
    ADCA.CH0.MUXCTRL = ADC_CH_MUXINT_TEMP_gc;

    adc_clear_existing_vars(ADC_CH_MUXINT_TEMP_gc);
}

void adc_clear_existing_vars(uint8_t muxsel) {
    ADCA.CH0.MUXCTRL = muxsel;
    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while (!(ADCA.INTFLAGS & ADC_CH0IF_bm)) {}
    ADCA.INTFLAGS = ADC_CH0IF_bm; // clear
    (void)ADCA.CH0.RES;
}



uint16_t sample_adc(uint8_t muxsel, uint8_t same_channel) {
    if (!same_channel) {
         adc_clear_existing_vars(muxsel);
    }
    // Real conversion
    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while (!(ADCA.INTFLAGS & ADC_CH0IF_bm)) {}
    uint16_t res = ADCA.CH0.RES;
    ADCA.INTFLAGS = ADC_CH0IF_bm; // clear
    return res;              
}
