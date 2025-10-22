#include <WiFi.h>

const char* ssid = "PicoServer";
const char* password = "12345678";
const char* serverIP = "192.168.42.1";  // default IP for SoftAP
int port = 4242;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  delay(2000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.printf("Connecting to AP: %s\n", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected!");

  if (client.connect(serverIP, port)) {
    Serial.println("Connected to server!");
    client.println("Hello from Pico Client!");
  } else {
    Serial.println("Failed to connect to server");
  }
}

void loop() {
  if (client.available()) {
    String msg = client.readStringUntil('\n');
    Serial.printf("Server says: %s\n", msg.c_str());
  }
  delay(1000);
}