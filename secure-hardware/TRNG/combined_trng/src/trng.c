
#include "hal.h"
#include "simpleserial.h"
#include <stdint.h>
#include <util/delay.h>

// #include <avr/iox128d4.h>

#define RTC_PER_VALUE  20000  
#define CHUNK_SIZE 249 // max chunk size for simpleserial2


// ---------- Declare headers ----------
static void rtc_init(void);
static void tcc0_init(void);
static inline void adc_clear_existing_vars(uint8_t muxsel);
static inline uint16_t sample_temp_sens(uint8_t muxsel);
static void adc_init_internal(void);
static uint8_t rng_get_byte_counter(void);
static uint8_t rng_get_byte_temp_sens(void);
static uint8_t rng_get_byte_vcc3_temp5(void);
static uint8_t get_random_bytes(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data);





// ---------- Bit-packer state ----------
static uint16_t bitbuf = 0;     // holds leftover bits between samples
static uint8_t bits_in_buf = 0;


// --- one-shot conversion helper on CH0 (with 1 dummy after MUX switch) ---
static inline void adc_clear_existing_vars(uint8_t muxsel) {
    ADCA.CH0.MUXCTRL = muxsel;
    // dummy conversion to settle
    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while (!(ADCA.INTFLAGS & ADC_CH0IF_bm)) {}
    (void)ADCA.CH0.RES;                   // read -> clears flag, discard

}


static inline uint16_t sample_temp_sens(uint8_t muxsel) {
    ADCA.CH0.MUXCTRL = muxsel;
    // real conversion
    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while (!(ADCA.INTFLAGS & ADC_CH0IF_bm)) {}
    return ADCA.CH0.RES;                  // read -> clears flag, keep
}

// ---------- RTC init ----------
static void rtc_init(void) {
    // 1. Enable 32 kHz oscillator
    OSC.CTRL |= OSC_RC32KEN_bm;              
    while (!(OSC.STATUS & OSC_RC32KRDY_bm)); // wait until oscillator is stable

    // 2. Route oscillator to RTC
    CLK.RTCCTRL = CLK_RTCSRC_RCOSC32_gc | CLK_RTCEN_bm;

    // 3. Configure RTC registers (always wait for SYNCBUSY between writes)

    while (RTC.STATUS & RTC_SYNCBUSY_bm);    // wait before writing PER
    RTC.PER = RTC_PER_VALUE;                 // set top value

    while (RTC.STATUS & RTC_SYNCBUSY_bm);    // wait before writing CNT
    RTC.CNT = 0;                             // reset counter

    while (RTC.STATUS & RTC_SYNCBUSY_bm);    // wait before writing CTRL
    RTC.CTRL = RTC_PRESCALER_DIV1_gc;        // enable and start

    while (RTC.STATUS & RTC_SYNCBUSY_bm);    // wait before clearing flags
    RTC.INTFLAGS = RTC_OVFIF_bm | RTC_COMPIF_bm; // clear overflow & compare flags
}


// ---------- TCC0 init ----------
static void tcc0_init(void) {
    TCC0.CTRLA = 0;      // stop timer during setup
    TCC0.PER   = 0x00FF; // max period (rollover at 65535)
    TCC0.CNT   = 0;      // reset counter
    TCC0.CTRLA = TC_CLKSEL_DIV1_gc; // run from system clock (32 MHz)
}

// ---------- ADC init for internal sources ----------
static void adc_init_internal(void) {
    ADCA.CTRLA = 0; // ensure disabled during config

    ADCA.PRESCALER = ADC_PRESCALER_DIV256_gc;                 // ~125 kHz @ 32 MHz CPU
    ADCA.REFCTRL   = ADC_REFSEL_INT1V_gc | ADC_TEMPREF_bm;    // 1.00V ref, enable temp path
    ADCA.CTRLB     = ADC_RESOLUTION_12BIT_gc;                 // unsigned, 12-bit

    ADCA.CH0.CTRL  = ADC_CH_INPUTMODE_INTERNAL_gc | ADC_CH_GAIN_1X_gc;

    ADCA.CTRLA     = ADC_ENABLE_bm;                           // enable last

    ADCA.CH0.MUXCTRL = ADC_CH_MUXINT_TEMP_gc;

    adc_clear_existing_vars(ADC_CH_MUXINT_TEMP_gc); // clear out any existing variables
}


/// ----------------- to change clear var to get mux switch between vcc and temp----///

// ---------- Make one byte: [7:5]=VCC/10 LSB3, [4:0]=TEMP LSB5 ----------
static uint8_t rng_get_byte_vcc3_temp5(void) {
	adc_clear_existing_vars(ADC_CH_MUXINT_SCALEDVCC_gc);// to change!!!!!!!!!!!!!
    uint16_t vcc  = sample_temp_sens(ADC_CH_MUXINT_SCALEDVCC_gc);
	adc_clear_existing_vars(ADC_CH_MUXINT_TEMP_gc);
    uint16_t temp = sample_temp_sens(ADC_CH_MUXINT_TEMP_gc);

    uint8_t top3 = (uint8_t)(vcc  & 0x07);    // 3 LSBs
    uint8_t low5 = (uint8_t)(temp & 0x1F);    // 5 LSBs
    return (uint8_t)((top3 << 5) | low5);
}



// Pack 5-bit ADC outputs into full bytes
static uint8_t rng_get_byte_temp_sens(void) {
    uint8_t four1 = (uint8_t)(sample_temp_sens(ADC_CH_MUXINT_TEMP_gc) & 0x0F);   // keep only 5 LSBs
    _delay_ms(10); 
    uint8_t four2 = (uint8_t)(sample_temp_sens(ADC_CH_MUXINT_TEMP_gc) & 0x0F);   // keep only 5 LSBs
    return (four1 << 4) | four2;
}

// Returns one random bit by waiting for the next RTC overflow
static uint8_t rng_get_byte_counter(void) {
    // Wait for overflow
    while (!(RTC.INTFLAGS & RTC_OVFIF_bm)) { ; }
    RTC.INTFLAGS = RTC_OVFIF_bm;
    uint8_t counter_value = TCC0.CNTL; 
    TCC0.CNT = 0; // reset TCC0 counter
    return counter_value; // read low byte of TCC0 counter
}






static uint8_t get_random_bytes(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data) {
    uint8_t N = data[0];   // how many bytes requested
    static uint8_t out[CHUNK_SIZE];

    for (uint16_t i = 0; i < N; i++) {
        switch (scmd) {
            case 0:
                out[i] = rng_get_byte_counter();     // method 0: TCC0 jitter
                break;
            case 1:
                out[i] = rng_get_byte_temp_sens();        // method 1: ADC noise
                break;
            case 2:
                out[i] = rng_get_byte_vcc3_temp5();      // method 2: combine both
                break;
            default:
                out[i] = 0xFF;                      // invalid selector → return dummy
                break;
        }
    }

    simpleserial_put('r', N, out);
    return 0;
}



int main(void) {
    platform_init();
    init_uart();
    trigger_setup();
	
	tcc0_init();
	rtc_init();
    adc_init_internal();

    simpleserial_init();
    simpleserial_addcmd('b', 1, get_random_bytes);

    while (1) {
        simpleserial_get();
    }
}

