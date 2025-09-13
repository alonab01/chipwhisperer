#ifndef ADC_UTILS_H
#define ADC_UTILS_H

#include <avr/io.h>
#include <stdint.h>

void adc_init_internal(void);
void adc_clear_existing_vars(uint8_t muxsel);
uint16_t sample_adc(uint8_t muxsel, uint8_t same_channel);

#endif
