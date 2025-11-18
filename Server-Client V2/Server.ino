// Author: Niraj Malokar
// Author: Mahin Akond
// Author: Jack Timmons

// ********** DEFINITIONS **********

#include <WiFi.h>

// Access Point credentials
const char* AP_SSID = "PicoServer";
const char* AP_PASS = "12345678";
const int PORT = 4242;

// Baud Rate for Serial Monitor
const int BAUD_RATE = 115200;

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

// Create a WiFi server on specified port
WiFiServer server(PORT);

void setup() {
  Serial.begin(BAUD_RATE);
  delay(2000);

  // Start Soft AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  // Print the IP address and SSID
  IPAddress IP = WiFi.softAPIP();
  Serial.printf("Access Point started: %s\n", AP_SSID);
  Serial.printf("AP IP address: %s\n", IP.toString().c_str());

  // Initialize LED pins
  for (int i = 0; i < NUMLEDS; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], HIGH); // LEDs on initially
  }

  server.begin();
}

//********** MAIN LOOP **********

void loop() {
  WiFiClient client = server.accept();
  if (client) {
    Serial.println("Client connected!");
    digitalWrite(ledPins[1], HIGH); // Middle LED on to indicate client connected
    digitalWrite(ledPins[0], LOW);
    digitalWrite(ledPins[2], LOW);

    while (client.connected()) {
      // toggle signal every 3 seconds
      if (millis() - last_toggle >= TOGGLE_INTERVAL) {
        // Sends a "1" or "0" to the client, switching back and forth every 3 seconds
        signalState = !signalState;
        client.println(signalState ? "1" : "0"); 
        Serial.printf("Sent signal: %d\n", signalState);
        last_toggle = millis();
      }

      // optionally read client messages
      if (client.available()) {
        String msg = client.readStringUntil('\n');
        msg.trim();
        Serial.printf("Received: %s\n", msg.c_str());
      }
    }

    // Stop client when disconnected
    client.stop();
    Serial.println("Client disconnected.");
  }
}

// ********** FUNCTION DEFINITIONS **********

/**
 * 
*/