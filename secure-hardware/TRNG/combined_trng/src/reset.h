#ifndef RESET_H
#define RESET_H

#include <stdint.h>

uint8_t cmd_reset(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data);
void mcu_software_reset(void);

#endif // RESET_H
