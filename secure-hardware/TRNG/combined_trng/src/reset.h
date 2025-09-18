#ifndef RESET_H
#define RESET_H

#include <stdint.h>

// SimpleSerial-compatible callback (no payload expected)
uint8_t cmd_reset(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data);

// Optional: call from anywhere in firmware
void mcu_software_reset(void);