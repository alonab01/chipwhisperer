#include "rng.h"
#include "adc_utils.h"
#include "simpleserial.h"
#include <util/delay.h>
#include <avr/io.h>

static uint16_t bitbuf = 0;     // buffer for leftover bits
static uint8_t bits_in_buf = 0; // how many bits are currently in buffer

uint8_t rng_get_byte_counter(void)
{
    // Wait for RTC overflow event
    while (!(RTC.INTFLAGS & RTC_OVFIF_bm))
    {
        ;
    }

    // Clear the RTC overflow flag
    RTC.INTFLAGS = RTC_OVFIF_bm;

    // Read the current TCC0 counter value and reset it
    uint8_t counter_value = TCC0.CNTL;
    TCC0.CNT = 0;

    // Return counter_value as the random byte
    return counter_value;
}



uint8_t rng_get_byte_vcc_temp(void)
{
    // Sample bytes based on VCC and TEMP ADC readings
    // We get 6 random bits per iteration (3 from VCC, 3 from TEMP)
    while (bits_in_buf < 8)
    {
        uint16_t vcc = sample_adc(ADC_CH_MUXINT_SCALEDVCC_gc, 0);
        uint16_t temp = sample_adc(ADC_CH_MUXINT_TEMP_gc, 0);

        // Take 3 bits from VCC, 3 bits from TEMP = 6 fresh bits
        uint8_t newbits = (uint8_t)((vcc & 0x07) << 3) | (temp & 0x07);

        // Push into bitbuf - saves bits for next iteration to reach a byte
        bitbuf |= ((uint16_t)newbits << bits_in_buf);
        bits_in_buf += 6;
    }

    // Extract 8 bits
    uint8_t out = (uint8_t)(bitbuf & 0xFF);
    bitbuf >>= 8;
    bits_in_buf -= 8;

    return out;
}

uint8_t rng_get_crc_byte(void)
{
    // Calculate the CRC
    for (uint8_t i = 0; i < CRC_BYTES; i++)
    {
        CRC.DATAIN = rng_get_byte_vcc_temp();
    }

    // XOR-ing all 4 bytes of the CRC result to get a single byte
    return CRC.CHECKSUM0 ^ CRC.CHECKSUM1 ^ CRC.CHECKSUM2 ^ CRC.CHECKSUM3;
}



uint8_t get_random_bytes(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data)
{
    // A simple serial command to get random bytes (based on scmd the type of the RNG)
    uint8_t N = data[0];
    static uint8_t out[CHUNK_SIZE];

    for (uint16_t i = 0; i < N; i++)
    {
        switch (scmd)
        {
        case 0:
            out[i] = rng_get_byte_counter();
            break;
        case 1:
            out[i] = rng_get_byte_vcc_temp();
            break;
        case 2:
            out[i] = rng_get_crc_byte();
            break;
        default:
            out[i] = 0x01;
            break;
        }
    }

    simpleserial_put('r', N, out);
    return 0;
}


uint16_t rng_get_counter(void)
{
    // Wait for RTC overflow event
    while (!(RTC.INTFLAGS & RTC_OVFIF_bm))
    {
        ;
    }

    // Clear the RTC overflow flag
    RTC.INTFLAGS = RTC_OVFIF_bm;

    // Read the current TCC0 counter value and reset it
    uint16_t counter_value = TCC0.CNT;
    TCC0.CNT = 0;

    // Return counter_value as the random byte
    return counter_value;
}

uint8_t get_counter_value(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data) {
    uint16_t v = rng_get_counter();
    uint8_t out[2] = { (uint8_t)(v >> 8),(uint8_t)(v & 0xFF) }; // LSB first
    simpleserial_put('r', 2, out);
    return 0;
}
