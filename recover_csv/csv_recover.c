/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "pico_lfs.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>

// Pico W devices use a GPIO on the WIFI chip for the LED,
// so when building for Pico W, CYW43_WL_GPIO_LED_PIN will be defined
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 250
#define FS_SIZE (256 * 1024)
#endif

static struct lfs_config * config;
static lfs_t lfs;

// Perform initialisation
int pico_led_init(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // For Pico W devices we need to initialise the driver etc
    return cyw43_arch_init();
#endif
}

// Turn the led on or off
void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // Ask the wifi "driver" to set the GPIO on or off
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#endif
}

// Read from CSV file we are using for documentation
void read_CSV(lfs_t *lfs, lfs_file_t *file){
    char read_text[5000] ="";
    lfs_file_open(lfs, file, "client.csv", LFS_O_RDONLY | LFS_O_CREAT); 
    lfs_file_read(lfs, file, &read_text, sizeof(read_text)-1); 
    lfs_file_close(lfs, file);

    printf("%s\n", read_text);
    return;
}

int main() {
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    } 
    
    //declaration of FS + initialize file
    lfs_t lfs;
    lfs_file_t file; 

    //config
    config = pico_lfs_init(PICO_FLASH_SIZE_BYTES - FS_SIZE, FS_SIZE);
    if(!config){
        printf("Out of memory\n");
    }

    //mount 
    int error = lfs_mount(&lfs, config); 
    if(error != LFS_ERR_OK){
        printf("error, did not work\n");
        lfs_format(&lfs, config);
        lfs_mount(&lfs, config);
    }
    sleep_ms(5000);

    read_CSV(&lfs, &file);

    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);
    while (true) {
        pico_set_led(true);
        sleep_ms(LED_DELAY_MS);
        pico_set_led(false);
        sleep_ms(LED_DELAY_MS);
    }
}