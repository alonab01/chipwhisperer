#include "aes-independant.h"
#include "hal.h"
#include "simpleserial.h"
#include <stdint.h>
#include <stdlib.h>

uint8_t get_pt(uint8_t *pt, uint8_t len)
{
	// Same exact function as in the attacked binary
	aes_indep_enc_pretrigger(pt);

	trigger_high();
	aes_indep_enc(pt);
	trigger_low();

	aes_indep_enc_posttrigger(pt);

	simpleserial_put('r', KEY_LENGTH, pt);
	return 0x00;
}

uint8_t get_key(uint8_t *pt, uint8_t len)
{
	// Sets the key - for acquiring traces
	aes_indep_key(pt);

	return 0x00;
}

int main(void)
{
	uint8_t secret_key[KEY_LENGTH] = {DEFAULT_KEY};

	platform_init();
	init_uart();
	trigger_setup();

	aes_indep_init();
	aes_indep_key(secret_key); // sets the key. size = 16 bytes

	simpleserial_init();
	simpleserial_addcmd('p', KEY_LENGTH, get_pt);
	simpleserial_addcmd('k', KEY_LENGTH, get_key);
	while (1)
		simpleserial_get();
}
