#include "reset.h"


void mcu_software_reset(void) {
    CCP = CCP_IOREG_gc;          // unlock protected I/O
    RST.CTRL = RST_SWRST_bm;     // software reset
    while (1) { }                // should never return
}

uint8_t cmd_reset(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data) {
    (void)cmd; (void)scmd; (void)dlen; (void)data;
    mcu_software_reset();
    return 0;                    // not reached
}
