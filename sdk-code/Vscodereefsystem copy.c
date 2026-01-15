#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

int main() {
    //for printing?
    stdio_init_all();

// Access Point credentials
    const char* AP_SSID = "PicoServer";
    const char* AP_PASS = "12345678";

// These 3 pins control LEDs on the Pico board
// All turn on when the server is running
// Middle remains on to indicate the client has connected
    const int ledPins[] = {11, 12, 13};
    const int NUMLEDS = sizeof(ledPins) / sizeof(ledPins[0]);

// Toggles between HIGH and LOW to send to client
    bool signalState = false;

// Timing the toggles - 3 second interval
    const unsigned long TOGGLE_INTERVAL = 3000;
    unsigned long last_toggle = 0;

// ********** SETUP **********

//initializes wifi card on pico
//this returns by itself so if it doesnt return loop will show fail
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }
//sets wifi chip to ap mode 
    cyw43_arch_enable_ap_mode(
        AP_SSID, //name of wifi
        AP_PASS, //password
        CYW43_AUTH_WPA2_AES_PSK //type of wifi we are using
    );

    printf("Access Point started: %s\n", AP_SSID);
    printf("AP IP address: %s\n", ipaddr_ntoa(&ip));
    
    for (int i = 0; i < NUMLEDS; i++) {
        gpio_init(pin); gpio_set_dir(ledpins[i], GPIO_OUT);
        gpio_put(pin, 1);
    }
//******** UDP STUFF *********

    while (true) {
        sleep_ms(1000);
    }

    cyw43_arch_deinit(); //cleans up 
    return 0;
}