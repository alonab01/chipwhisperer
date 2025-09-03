// main.c — CW303 (ATxmega128D4): read a free-running timer and return it
// Uses ChipWhisperer HAL + SimpleSerial (no extra libs to install).
// Build: make PLATFORM=CW303

#include "hal.h"
#include "simpleserial.h"
#include <avr/iox128d4.h>
#include <avr/io.h>
#include <stdint.h>

// ----- Timer setup: TCC0 free-runs at CPU clock (default 32 MHz from HAL) -----
static void timer_init(void) {
    // Ensure timer stopped while configuring
    TCC0.CTRLA = 0;
    TCC0.CTRLB = 0;
    TCC0.CTRLC = 0;
    TCC0.CTRLD = 0;
    TCC0.CTRLE = 0;
    TCC0.INTCTRLA = 0;
    TCC0.INTCTRLB = 0;

    TCC0.PER = 0xFFFF;      // 16-bit rollover
    TCC0.CNT = 0;
    TCC0.CTRLA = TC_CLKSEL_DIV1_gc;   // count at CPU clock (HAL sets CPU=32 MHz)
}

// ----- SimpleSerial command: host sends 'r' -> we return 2 bytes (LSB first) -----
static uint8_t cmd_read_timer(uint8_t *data, uint16_t len) {
    (void)data; (void)len;

    uint16_t cnt = TCC0.CNT;               // sample the timer "now"
    uint8_t out[2] = { (uint8_t)(cnt & 0xFF), (uint8_t)(cnt >> 8) };
    simpleserial_put('r', 2, out);         // send back 2 bytes

    return 0x00;
}

// Optional: read just the LSB (handy for quick bias checks)
static uint8_t cmd_read_lsb(uint8_t *data, uint16_t len) {
    (void)data; (void)len;

    uint8_t bit = (uint8_t)(TCC0.CNT & 1);
    simpleserial_put('b', 1, &bit);
    return 0x00;
}

int main(void)
{
    platform_init();   // ChipWhisperer HAL: sets 32 MHz clock, disables JTAG, etc.
    init_uart();       // set up UART on USARTC0 to talk SimpleSerial
    trigger_setup();   // not strictly needed here, but keeps HAL happy

    timer_init();

    simpleserial_init();
    simpleserial_addcmd('r', 0, cmd_read_timer);  // get 16-bit timer value
    simpleserial_addcmd('b', 0, cmd_read_lsb);    // get 1-byte LSB

    while (1) {
        simpleserial_get();   // process incoming commands
    }
}
