// Author: Jack Timmons
// Version: Apr 2026

/**
 * Code for igniting the EMatch when the signal is received 
 */
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define ALTIMETERPIN 28
#define EMATCHPIN 10

/**
 * Main function
 */
int main(){
    init();
    
    while(true){
        if (gpio_get(ALTIMETERPIN)){
            gpio_put(EMATCHPIN, 1);
        }
    }

    return 0;
}


/**
 * Fuction to set up all pins and their directions 
 * (this would be ~50 lines without the libraries)
 */
void init(){
    stdio_init_all();

    init_ematch_pin();
    init_altimeter_pin();
}

void init_ematch_pin(){
    gpio_init(EMATCHPIN);
    gpio_set_dir(EMATCHPIN, GPIO_OUT);
}

void init_altimeter_pin(){
    gpio_init(ALTIMETERPIN);
    gpio_set_dir(ALTIMETERPIN, GPIO_IN);

    // Altimeter is pull up; 1 when IN == 0, 0 when IN == 1
    gpio_pull_up(ALTIMETERPIN);
}
