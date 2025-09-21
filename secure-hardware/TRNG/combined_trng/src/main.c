#include "hal.h"
#include "simpleserial.h"
#include "rng.h"
#include "init.h"
#include "adc_utils.h"
#include "reset.h"
#include <stdio.h>

int main(void)
{
    // Initialize hardware required for the ChipWhisperer platform
    platform_init();
    init_uart();
    trigger_setup();

    // Initialize our TRNG components
    tcc0_init();
    rtc_init();
    crc_init();
    adc_init_internal();

    // Initialize Simple Serial (our interface to the PC)
    simpleserial_init();

    // Print the BOOT message to indicate readiness of the device (for restart tests)
    simpleserial_put('r', 8, (uint8_t *)"##BOOT##");

    // Add our Sipmle Serial commands
    simpleserial_addcmd('b', 1, get_random_bytes);
    simpleserial_addcmd('x', 0, cmd_reset);

    while (1)
    {
        simpleserial_get();
    }
}
