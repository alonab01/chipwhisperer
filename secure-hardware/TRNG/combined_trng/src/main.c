#include "hal.h"
#include "simpleserial.h"
#include "rng.h"
#include "timers.h"
#include "adc_utils.h"
#include "reset.h"

int main(void) {
    platform_init();
    init_uart();
    trigger_setup();
    
    tcc0_init();
    rtc_init();
    adc_init_internal();

    simpleserial_init();
    simpleserial_addcmd('b', 1, get_random_bytes);
    simpleserial_addcmd('x', 0, cmd_reset);         // reset


    while (1) {
        simpleserial_get();
    }
}
