// Author: Niraj Malokar
// Author: Mahin Akond

#include <WiFi.h>

// These are our current example Wi-Fi credentials
const char* ssid = "PicoServer";
const char* password = "12345678";
const char* serverIP = "192.168.42.1";
int port = 4242;

// Sets up our client variables
WiFiClient client;
unsigned long lastMessageTime = 0;
bool wasConnected = false;

void setup() {
  // This is our baud rate for Serial Monitor
  Serial.begin(115200);
  delay(2000);

  // Initialize LED pin as OUTPUT pin, and turn it off initially
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Call our functions to connect to Wi-Fi and server
  Serial.println("Starting client...");
  connectToWiFi();
  connectToServer();
}

void loop() {
  // Read messages from server if connected
  if (client.connected()) {
    if (client.available()) {
      // Read the message from the server and print them to serial monitor
      String msg = client.readStringUntil('\n');
      msg.trim();
      Serial.printf("Server says: %s\n", msg.c_str());

      // Reset timer when we receive any message
      lastMessageTime = millis();

      // Control built-in LED based on server message
      if (msg == "1") {
        digitalWrite(LED_BUILTIN, HIGH);
      } else if (msg == "0") {
        digitalWrite(LED_BUILTIN, LOW);
      }
    }

    // Simple check: if no messages for 10 seconds, assume that we are disconnected from server and stop connection
    if (millis() - lastMessageTime > 10000) {
      Serial.println("No messages for 10 seconds - assuming disconnected");
      client.stop();
      wasConnected = false;
      digitalWrite(LED_BUILTIN, LOW);
    }
  } 
  else {
    // Not connected - try to reconnect every 15 seconds (This could be less time maybe?)
    if (millis() - lastMessageTime > 15000) {
      Serial.println("Attempting to reconnect...");
      
      // Check WiFi first
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnecting WiFi...");
        WiFi.begin(ssid, password);
        delay(2000);
      }
      
      // Try to connect to server
      if (client.connect(serverIP, port)) {
        Serial.println("Reconnected to server!");
        client.println("Hello from Pico Client!");
        lastMessageTime = millis();
        wasConnected = true;
      } else {
        Serial.println("Failed to reconnect");
        lastMessageTime = millis(); // Reset timer to try again in 15 seconds
      }
    }
  }

  delay(100);
}

// Function to connect to Wi-Fi
void connectToWiFi() {
  // Set Wi-Fi to station mode and attempt connection
  Serial.printf("Connecting to Wi-Fi: %s\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

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

// Function to connect to TCP server
void connectToServer() {
  Serial.printf("Connecting to server %s:%d\n", serverIP, port);
  
  // Try to connect to server
  if (client.connect(serverIP, port)) {
    Serial.println("Connected to server!");
    client.println("Hello from Pico Client!");
    lastMessageTime = millis();
    wasConnected = true;
  } else {
    Serial.println("Failed to connect to server");
    lastMessageTime = millis(); // Start reconnection attempts
  }
}