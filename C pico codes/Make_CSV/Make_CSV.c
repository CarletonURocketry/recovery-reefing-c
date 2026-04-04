#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico_lfs.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 250
#endif

#define FS_SIZE (256 * 1024)

static struct lfs_config * config;
static lfs_t lfs;

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

void write_to_CSV(char str[], lfs_t *lfs, lfs_file_t *file){
    printf("%d\n", strlen(str));
    lfs_file_open(lfs, file, "boot_count", LFS_O_WRONLY | LFS_O_CREAT); 
    lfs_file_write(lfs, file, str, strlen(str)); // Write to system
    lfs_file_close(lfs, file);
    return;
} 

void ReadCSV(lfs_t *lfs, lfs_file_t *file){

    char read_text[5000] ="";
    lfs_file_open(lfs, file, "boot_count", LFS_O_RDONLY | LFS_O_CREAT); 
    lfs_file_read(lfs, file, &read_text, sizeof(read_text)-1); 
    lfs_file_close(lfs, file);

    printf("%s\n", read_text);
    return;
}

void reset_csv(lfs_t *lfs, lfs_file_t *file){
    lfs_file_open(lfs, file, "boot_count", LFS_O_TRUNC); 
    lfs_file_close(lfs, file); 
    return;  
}

int main()
{
    stdio_init_all();
    sleep_ms(5000);

    //declaration of FS + initialize file
    lfs_t lfs;
    lfs_file_t file; 

    //establish timer
    // uint32_t start, stop, sometime; 

    // char line_buffer[128];

    //config
    config = pico_lfs_init(PICO_FLASH_SIZE_BYTES - FS_SIZE, FS_SIZE);
    if(!config){
        printf("Out of memory\n");
    }

    //mount 
    int err = lfs_mount(&lfs, config); 
    if(err != LFS_ERR_OK){
        printf("err did not work\n");
        lfs_format(&lfs, config);
        lfs_mount(&lfs, config);
    }
    
    char hello[] = "lets switch it up a bit\n";
    char bye[] ="";
    // read current count
    uint32_t boot_count = 0;
    

    //lfs_file_open(&lfs, &file, "boot_count", LFS_O_WRONLY | LFS_O_CREAT); 
    // lfs_file_write(&lfs, &file, &hello, sizeof(hello)); // Write to system
    // lfs_file_close(&lfs, &file);

    // lfs_file_rewind(&lfs, &file);
    //lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDONLY | LFS_O_CREAT); 
    //lfs_file_read(&lfs, &file, &bye, 8000); 
    //lfs_file_close(&lfs, &file);
    reset_csv(&lfs, &file);
    sleep_ms(50);
    write_to_CSV("This is the written test", &lfs, &file);
    sleep_ms(50);
    ReadCSV(&lfs, &file);

    // release any resources we were using
    lfs_unmount(&lfs);

    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);
    while (true) {
        pico_set_led(true);
        sleep_ms(LED_DELAY_MS);
        pico_set_led(false);
        sleep_ms(LED_DELAY_MS);
    }
}