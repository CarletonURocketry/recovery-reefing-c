#include <stdio.h>
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

int main()
{
    stdio_init_all();
    sleep_ms(5000);

    //declaration of FS + initialize file
    lfs_t lfs;
    lfs_file_t file; 

    //establish timer
    uint32_t start, stop, sometime; 

    char line_buffer[128];

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

    //open file + fail check
    /*err = lfs_file_open(&lfs, &file, "makefile.csv", LFS_O_RDWR | LFS_O_CREAT);
    printf("file opened\n");
    if (!err){
        printf("Error opening file to write\n");
    }

    //Read file (See if csv saved)
    lfs_file_read(&lfs, &file, "makefile.csv", sizeof(32));

    //Write to file
    lfs_file_write(&lfs, &file, "This is a test", sizeof(32));
    printf("written to file\n");

    lfs_file_close(&lfs, &file);
    printf("file closed\n");

    //close file
    lfs_file_close(&lfs, &file);
    printf("file closed\n");

    lfs_unmount(&lfs); */

    // read current count
    uint32_t boot_count = 0;
    lfs_file_open(&lfs, &file, "boot_count.csv", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    // update boot count
    boot_count += 1;
    lfs_file_rewind(&lfs, &file);
    lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&lfs, &file);

    // release any resources we were using
    lfs_unmount(&lfs);

    // print the boot count
    printf("boot_count: %d\n", boot_count);

    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);
    while (true) {
        pico_set_led(true);
        sleep_ms(LED_DELAY_MS);
        pico_set_led(false);
        sleep_ms(LED_DELAY_MS);
    }
}