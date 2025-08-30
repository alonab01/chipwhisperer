#include "aes-independant.h"
#include "hal.h"
#include "simpleserial.h"
#include <stdint.h>
#include <stdlib.h>

uint8_t get_pt(uint8_t* pt, uint8_t len)
{
    aes_indep_enc_pretrigger(pt);

	trigger_high();
	aes_indep_enc(pt); /* encrypting the data block */
	trigger_low();

    aes_indep_enc_posttrigger(pt);

	simpleserial_put('r', 16, pt);
	return 0x00;
}

int main(void)
{
	uint8_t secret_key[KEY_LENGTH] = "This is Correct!";

    platform_init();
    init_uart();
    trigger_setup();

	aes_indep_init();
	aes_indep_key(secret_key); // sets the key. size = 16 bytes

	simpleserial_init();
    simpleserial_addcmd('p', 16,  get_pt);
    while(1)
        simpleserial_get();
}
