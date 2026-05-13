// Author: Jack Timmons
// Version: Apr 2026

/**
 * Code for igniting the EMatch when the signal is received 
 */
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define ALTIMETERPIN 28
#define EMATCHPIN 10
#define LED1 11
#define LED2 12
#define LED3 13

void init_leds(){
    gpio_init(LED1);
    gpio_set_dir(LED1, GPIO_OUT);
    gpio_init(LED2);
    gpio_set_dir(LED2, GPIO_OUT);
    gpio_init(LED3);
    gpio_set_dir(LED3, GPIO_OUT);
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

void init(){
    stdio_init_all();

    init_ematch_pin();
    init_altimeter_pin();
}


/**
 * Main function
 */
int main(){

    int led = 0;

    init();
    
    while(true){
        gpio_put(LED1, 0);
        
        // if (led = 0){
        //     gpio_put(LED1, 1);
        //     gpio_put(LED3, 0);
        //     led = 1;
        // }else if (led = 1){
        //     gpio_put(LED2, 1);
        //     gpio_put(LED1, 0);
        //     led = 2;
        // }else if (led = 2){
        //     gpio_put(LED3, 1);
        //     gpio_put(LED2, 0);
        //     led = 0;
        // }
        
        if (gpio_get(ALTIMETERPIN)){
            gpio_put(EMATCHPIN, 1);
        }

        sleep_ms(1000);

    }

    return 0;
}


/**
 * Fuction to set up all pins and their directions 
 * (this would be ~50 lines without the libraries)
 */







