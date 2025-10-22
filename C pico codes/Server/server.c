#include <WiFi.h>

const char* ap_ssid = "PicoServer";
const char* ap_pass = "12345678";  // must be at least 8 chars
int port = 4242;

WiFiServer server(port);

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Start Soft AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_pass);

  IPAddress IP = WiFi.softAPIP();
  Serial.printf("Access Point started: %s\n", ap_ssid);
  Serial.printf("AP IP address: %s\n", IP.toString().c_str());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Client connected!");
    while (client.connected()) {
      if (client.available()) {
        String msg = client.readStringUntil('\n');
        Serial.printf("Received: %s\n", msg.c_str());
        client.println("Hello from Pico AP Server!");
      }
    }
    client.stop();
    Serial.println("Client disconnected.");
  }
}
