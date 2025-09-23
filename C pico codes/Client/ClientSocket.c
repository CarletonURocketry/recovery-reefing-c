#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include "pico/stdlib.h" // pico GPIO functions
#include "pico/cyw43_arch.h" // for Wi-Fi functions

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/pwm.h" // for PWM functionality
#include "hardware/clocks.h"
#include <time.h>

#define FLASH_TARGET_OFFSET (1024 * 1024) // 1 MB offset, should be good enough
#define FLASH_SECTOR_SIZE 4096
#define LOG_BUFFER_SIZE 4096

// Set the LEDs and PINS
#define BUZZER_PIN 25
#define LED1_PIN 11
#define LED2_PIN 12
#define LED3_PIN 13
#define CHARGE_PIN 28
#define PORT 80
// Access info
#define SSID "SSID"
#define PASS "PASSWORD"


/////////////////////// CSV Flight Log Codes //////////////////

char log_buffer[LOG_BUFFER_SIZE];
size_t log_buffer_index = 0;

void clear_log_buffer() {
    memset(log_buffer, 0, LOG_BUFFER_SIZE);
    log_buffer_index = 0;
}


void log_event(const char *event) {
    // Get the current time (relative timestamp since boot)
    uint64_t timestamp = to_us_since_boot(get_absolute_time());

    // Format the log entry
    char log_entry[128];
    snprintf(log_entry, sizeof(log_entry), "%llu,%s\n", timestamp, event);

    // Append the log entry to the buffer
    if (log_buffer_index + strlen(log_entry) < LOG_BUFFER_SIZE) {
        strcat(log_buffer + log_buffer_index, log_entry);
        log_buffer_index += strlen(log_entry);
    } else {
        printf("Log buffer full! Event not logged: %s\n", event);
    }
}

void save_log_to_flash() {
    // Disable interrupts to avoid conflicts during Flash operations
    uint32_t interrupts = save_and_disable_interrupts();

    // Erase the target sector
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);

    // Pad the log buffer to a multiple of 256 bytes
    size_t padded_size = (log_buffer_index + 255) & ~255; // Round up to the nearest 256 bytes
    if (padded_size > FLASH_SECTOR_SIZE) {
        padded_size = FLASH_SECTOR_SIZE; // Ensure it doesn't exceed the sector size
    }

    uint8_t padded_buffer[FLASH_SECTOR_SIZE]; // Use a fixed-size buffer
    memset(padded_buffer, 0, padded_size); // Fill the buffer with zeros
    memcpy(padded_buffer, log_buffer, log_buffer_index); // Copy the log data into the padded buffer

    // Write the padded log buffer to Flash
    flash_range_program(FLASH_TARGET_OFFSET, padded_buffer, padded_size);

    // Restore interrupts
    restore_interrupts(interrupts);

    printf("Log saved to Flash!\n");
}

void read_log_from_flash() {
    // Pointer to the Flash memory location
    const char *flash_data = (const char*)(XIP_BASE + FLASH_TARGET_OFFSET);

    // Create a buffer to hold the log data
    char buffer[LOG_BUFFER_SIZE + 1]; // +1 for null terminator
    memcpy(buffer, flash_data, LOG_BUFFER_SIZE);
    buffer[LOG_BUFFER_SIZE] = '\0'; // Null-terminate the buffer

    // Print the log data
    printf("Log from Flash:\n%s", buffer);
}

////////////////////////////// Buzzer Codes //////////////////////////////////////////

void buzzer() {
    // Configure GPIO 25 as a PWM output
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);

    // Get the PWM slice and channel for GPIO 25
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint channel_num = pwm_gpio_to_channel(BUZZER_PIN);

    // Set the PWM frequency
    uint32_t clock_freq = 125000000; // Pico's system clock frequency (125 MHz)
    uint32_t wrap_value = clock_freq / 4000; // 4 kHz frequency
    pwm_set_wrap(slice_num, wrap_value);

    // Set the duty cycle (50% for a square wave)
    pwm_set_chan_level(slice_num, channel_num, wrap_value / 2);

    // Enable the PWM slice
    pwm_set_enabled(slice_num, true);

    // Keep the buzzer on for 1 second
    sleep_ms(1000);

    // Disable the PWM slice to stop the buzzer
    pwm_set_enabled(slice_num, false);
}

void play_note(uint freq, uint32_t duration_ms) {
    // Configure the GPIO pin for PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    // Set PWM frequency
    uint32_t clock_freq = 125000000; // 125 MHz system clock
    uint32_t wrap_value = clock_freq / freq; // Calculate wrap value for desired frequency
    pwm_set_wrap(slice_num, wrap_value);

    // Set the duty cycle (50% for a square wave)
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER_PIN), wrap_value / 2);

    // Enable the PWM
    pwm_set_enabled(slice_num, true);

    // Sleep for the duration of the note
    sleep_ms(duration_ms);

    // Disable the PWM after the note duration
    pwm_set_enabled(slice_num, false);
}

void quaterPounder() {
    play_note(G4, 400); 
    sleep_ms(100);     
    play_note(E4, 400); 
    sleep_ms(100);
    play_note(G4, 400); 
    sleep_ms(100);
    play_note(G4, 400);
    sleep_ms(100);
    play_note(G4, 400); 
    sleep_ms(500);      
}

///////////////////////////////////////////////////////////////////////////////////


void set_leds(bool led1, bool led2, bool led3) { //turns led off and on. 
    gpio_put(LED1_PIN, led1);
    gpio_put(LED2_PIN, led2);
    gpio_put(LED3_PIN, led3);
}

void connectWIFI() {
    if (cyw43_arch_init()) {
        printf("Wi-Fi module failed\n");
        return;
    }
    cyw43_arch_enable_sta_mode();

    int retries = 3;
    while (retries--) {
        if (cyw43_arch_wifi_connect_blocking(SSID, PASS, CYW43_AUTH_WPA2_MIXED) == CYW43_SUCCESS) {
            printf("Wi-Fi connected\n");
            log_event("Wifi connected");
            set_leds(0, 1, 0);
            return;
        }
        printf("Wi-Fi connection failed. Retrying...\n");
        sleep_ms(5000); // Wait 5 seconds before retrying
    }

    printf("Wi-Fi connection failed after retries\n");
}

int main(){

    gpio_init(CHARGE_PIN);
	gpio_set_dir(CHARGE_PIN, GPIO_IN);
    gpio_put(CHARGE_PIN,0);

	gpio_init(LED1_PIN);
	gpio_init(LED2_PIN);
	gpio_init(LED3_PIN);
	gpio_set_dir(LED1_PIN, GPIO_OUT);
	gpio_set_dir(LED2_PIN, GPIO_OUT);
	gpio_set_dir(LED3_PIN, GPIO_OUT);
	
	gpio_init(PICO_PIN);
	gpio_set_dir(PICO_PIN, GPIO_OUT);
	
	set_leds(0,0,0);

    buzzer();

    connectWIFI();
    sleep_ms(2000);
    set_leds(0,0,0);

	//create a socket
	int networkSocket;
	networkSocket = socket(AF_INET, SOCK_STREAM, 0);
	
	//specify the address for the socket and port, to be stored in AF_INET
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT); //the parameter for port is kind of weird so we use htons() function to convert the integer in to whatever port 	number we require it to work as.
	
	server_address.sin_addr.s_addr = inet_addr("192.168.4.1");

	int connection_status = connect(networkSocket, (struct sockaddr *) &server_address, sizeof(server_address));  
	//connect() returns an integer to confirm successful connection, 0 means all Gucci, -1 means oh no.
	
	if (connection_status == -1){
	printf("Connection failed");
    set_leds(1,0,1);
    }else{
    set_leds(1, 0, 0);
    sleep_ms(1000);
    set_leds(1, 1, 0);
    sleep_ms(1000);
    set_leds(1, 1, 1);
    sleep_ms(2000);
    set_leds(0, 0, 0);
    printf("Server said go time B)\n");
    log_event("Connected to Server");
    }
	
	//receive data from the server
	char server_response[256];
	int responseByte = recv(networkSocket, &server_response, sizeof(server_response), 0);

    if (responseByte>0){
        server_response[responseByte] = '\0'; //null termination to prevent buffer overflow

        printf("Server sent: %s\n", server_response);

        if (strncmp(server_response,"True",4)==0){
            gpio_put(CHARGE_PIN,1); //send charge to e-match
            set_leds(1,1,1);
            log_event("Charge sent");
        }else if (strncmp(server_response,"False",5)==0)
        {
            set_leds(0,1,0);
        }else{
            set_leds(0,0,0);
        }
    }

	
	printf("The server sent the data:%s\n", server_response);
	
	//close the socket
	close(networkSocket);

    save_log_to_flash();
    read_log_from_flash();

	return 0;
}