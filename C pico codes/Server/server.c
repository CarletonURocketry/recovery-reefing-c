// Author: Niraj Malokar
// Author: Mahin Akond

#include <WiFi.h>

// This is our current example Access Point credentials
const char* ap_ssid = "PicoServer";
const char* ap_pass = "12345678";
int port = 4242;

// Create a WiFi server on specified port
WiFiServer server(port);

// These 3 pins control LEDs on the Pico board
// These pins were used for debugging, currently they all turn on when the server is running
// Only the middle one remains on to indicate the client has connected
int ledPins[] = {11, 12, 13};
int numLeds = sizeof(ledPins) / sizeof(ledPins[0]);

// This variable toggles between HIGH and LOW to send to client
bool signalState = false;

// This is used for timing the toggles, delay() was blocking server functionality
unsigned long lastToggle = 0;
unsigned long toggleInterval = 3000; // 3 seconds

void setup() {
  // This is our baud rate for Serial Monitor
  Serial.begin(115200);
  delay(2000);

  // Start Soft AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_pass);

  // Print the IP address and SSID
  IPAddress IP = WiFi.softAPIP();
  Serial.printf("Access Point started: %s\n", ap_ssid);
  Serial.printf("AP IP address: %s\n", IP.toString().c_str());

  // Initialize LED pins
  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], HIGH); // LEDs on initially
  }

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Client connected!");
    digitalWrite(ledPins[1], HIGH); // Middle LED on to indicate client connected
    digitalWrite(ledPins[0], LOW);
    digitalWrite(ledPins[2], LOW);

    while (client.connected()) {
      // toggle signal every 3 seconds
      if (millis() - lastToggle >= toggleInterval) {
        // Sends a "1" or "0" to the client, switching back and forth every 3 seconds
        signalState = !signalState;
        client.println(signalState ? "1" : "0"); 
        Serial.printf("Sent signal: %d\n", signalState);
        lastToggle = millis();
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
