#include "reset.h"
#include <avr/io.h>

void mcu_software_reset(void)
{
    // Perform a software reset using the RST.CTRL register
    CCP = CCP_IOREG_gc;      // unlock protected I/O
    RST.CTRL = RST_SWRST_bm; // software reset
    while (1)
    {
    }
}

uint8_t cmd_reset(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data)
{
    // To avoid compiler warnings about unused parameters
    (void)cmd;
    (void)scmd;
    (void)dlen;
    (void)data;
    mcu_software_reset();
    return 0;
}
