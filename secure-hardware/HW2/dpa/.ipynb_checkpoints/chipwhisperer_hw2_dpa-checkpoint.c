#include "hal.h"
#include "simpleserial.h"
#include <stdint.h>
#include <stdlib.h>

uint8_t v_func(uint8_t* msg, uint8_t len)
{
    uint8_t res = 255;
    if(len != 9) {
        simpleserial_put('r', 1, &res);
        return 0x00;
    }

    /* < traces start here > */
    trigger_high(); 
    ///// change here
    uint8_t sum = 0;
    for(int i=0; i<len; i++) {
        uint8_t num = (msg[i] - '0');
        num *= i % 2 + 1;  //<=== I added +1 here
        num = num % 10 + num / 10;  //<===
        sum += num;
    }
    res = sum % 10 == 0;  //<=== I switch to equal
    /////
	
    /* < traces end here > */
    trigger_low(); 

    
    res = res ? 'Y' : 'N';
    /* < send back result > */
    simpleserial_put('r', 1, &res); 
    return 0x00;
}

uint8_t f_func(uint8_t* msg, uint8_t len)
{
    uint8_t res = 255;
    if(len != 8) {
        simpleserial_put('r', 1, &res);
        return 0x00;
    }

    /* < traces start here > */
    trigger_high(); 
    ///// change here
    uint8_t sum = 0;
    for(int i=0; i<len; i++) {
        uint8_t num = (msg[i] - '0');
        num *= i % 2 + 1;
        num = num % 10 + num / 10;
        sum += num;
    }
    res = (200-sum) % 10;  //<===
    /////
    /* < traces end here > */
    trigger_low(); 

    
    res += '0';
    /* < send back result > */
    simpleserial_put('r', 1, &res);
    return 0x00;
}

int main(void)
{
    platform_init();
    init_uart();
    trigger_setup();
	simpleserial_init();
    
    simpleserial_addcmd('a', 9, v_func);
    simpleserial_addcmd('b', 8, f_func);
    while(1)
        simpleserial_get();
}
