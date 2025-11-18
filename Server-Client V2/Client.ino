// Author: Niraj Malokar
// Author: Mahin Akond
// Author: Jack Timmons

// ********** DEFINITIONS **********

#include <WiFi.h>

// Wi-Fi credentials
const char* SSID = "PicoServer";
const char* PASSWORD = "12345678";
const char* SERVER_IP = "192.168.42.1";
const int PORT = 4242;

// Client variables
WiFiClient client;
unsigned long last_message_time = 0;
bool was_connected = false;

// Baud rate for Serial Monitor
const int BAUD_RATE = 115200;

// ********** SETUP **********

void setup() {
  Serial.begin(BAUD_RATE);
  delay(2000);

  // Initialize LED pin as OUTPUT pin, off initially
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Call our functions to connect to Wi-Fi and server
  Serial.println("Starting client...");
  connectToWiFi();
  connectToServer();
}

// ********** MAIN LOOP ***********

void loop() {
  // Read messages from server if connected
  if (client.connected()) {
    if (client.available()) {
      // Read the message from the server and print them to serial monitor
      String msg = client.readStringUntil('\n');
      msg.trim();
      Serial.printf("Server says: %s\n", msg.c_str());

      // Reset timer when we receive any message
      last_message_time = millis();

      // Control built-in LED based on server message
      if (msg == "1") {
        digitalWrite(LED_BUILTIN, HIGH);
      } else if (msg == "0") {
        digitalWrite(LED_BUILTIN, LOW);
      }
    }

    // Simple check: if no messages for 10 seconds, assume that we are disconnected from server and stop connection
    if (millis() - last_message_time > 10000) {
      Serial.println("No messages for 10 seconds - assuming disconnected");
      client.stop();
      was_connected = false;
      digitalWrite(LED_BUILTIN, LOW);
    }
  } 
  else {
    // Not connected - try to reconnect every 15 seconds (This could be less time maybe?)
    if (millis() - last_message_time > 15000) {
      Serial.println("Attempting to reconnect...");
      
      // Check WiFi first
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnecting WiFi...");
        WiFi.begin(SSID, PASSWORD);
        delay(2000);
      }
      
      // Try to connect to server
      if (client.connect(SERVER_IP, PORT)) {
        Serial.println("Reconnected to server!");
        client.println("Hello from Pico Client!");
        last_message_time = millis();
        was_connected = true;
      } else {
        Serial.println("Failed to reconnect");
        last_message_time = millis(); // Reset timer to try again in 15 seconds
      }
    }
  }

  delay(100);
}

// ********** FUNCTION DEFINITIONS **********

/**
 * Connect to Wi-Fi
*/
void connectToWiFi() {
  // Set Wi-Fi to station mode and attempt connection
  Serial.printf("Connecting to Wi-Fi: %s\n", SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  // Wait for connection with timeout
  for(int i = 0; i < 20; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWi-Fi connected!");
      Serial.printf("Client IP: %s\n", WiFi.localIP().toString().c_str());
      return;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWi-Fi connection failed!");
}

/**
 * Connect to TCP Server
*/ 
void connectToServer() {
  Serial.printf("Connecting to server %s:%d\n", SERVER_IP, PORT);
  
  // Try to connect to server
  if (client.connect(SERVER_IP, PORT)) {
    Serial.println("Connected to server!");
    client.println("Hello from Pico Client!");
    last_message_time = millis();
    was_connected = true;
  } else {
    Serial.println("Failed to connect to server");
    last_message_time = millis(); // Start reconnection attempts
  }
}

