#ifndef RNG_H
#define RNG_H

#include <stdint.h>

#define CHUNK_SIZE 249

uint8_t rng_get_byte_counter(void);
uint8_t rng_get_byte_temp_sens(void);
uint8_t rng_get_byte_vcc3_temp5(void);

uint8_t get_random_bytes(uint8_t cmd, uint8_t scmd, uint8_t dlen, uint8_t *data);

#endif
