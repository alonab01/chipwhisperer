#include "adc_utils.h"
#include <avr/io.h>

void adc_init_internal(void)
{
    // Disable ADC before configuration
    ADCA.CTRLA = 0;

    // Configure ADC settings
    ADCA.PRESCALER = ADC_PRESCALER_DIV256_gc;
    ADCA.REFCTRL = ADC_REFSEL_INT1V_gc | ADC_TEMPREF_bm;
    ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc;

    // Configure ADC channel 0
    ADCA.CH0.CTRL = ADC_CH_INPUTMODE_INTERNAL_gc | ADC_CH_GAIN_1X_gc;

    // Enable ADC and select temperature sensor as input (arbitrary choice for startup)
    ADCA.CTRLA = ADC_ENABLE_bm;
    ADCA.CH0.MUXCTRL = ADC_CH_MUXINT_TEMP_gc;

    adc_clear_existing_vars(ADC_CH_MUXINT_TEMP_gc);
}

void adc_clear_existing_vars(uint8_t muxsel)
{
    // Clear the ADC to remove any existing state
    ADCA.CH0.MUXCTRL = muxsel;

    // Perform first ADC read (since first read produce garbage values)
    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while (!(ADCA.INTFLAGS & ADC_CH0IF_bm))
    {
    }
    ADCA.INTFLAGS = ADC_CH0IF_bm; // clear

    (void)ADCA.CH0.RES;
}

uint16_t sample_adc(uint8_t muxsel, uint8_t same_channel)
{
    // Avoid clearing if we are sampling the same channel again
    if (!same_channel)
    {
        // Switching the channel, clear existing variables
        adc_clear_existing_vars(muxsel);
    }

    // Start Analog-to-Digital conversion
    ADCA.CH0.CTRL |= ADC_CH_START_bm;

    // Wait for conversion to complete
    while (!(ADCA.INTFLAGS & ADC_CH0IF_bm))
    {
    }

    // Reading the result and clear the interrupt flag
    uint16_t res = ADCA.CH0.RES;
    ADCA.INTFLAGS = ADC_CH0IF_bm;

    // Return the read result
    return res;
}
