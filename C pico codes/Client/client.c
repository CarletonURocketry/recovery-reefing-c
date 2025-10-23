
#include <WiFi.h>

const char* ssid = "PicoServer";
const char* password = "12345678";
const char* serverIP = "192.168.42.1";  // default IP for SoftAP
int port = 4242;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(LED_BUILTIN, OUTPUT); // pico built-in LED

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.printf("Connecting to AP: %s\n", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(1000);                      // wait for a second
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    delay(1000);
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected!");

  if (client.connect(serverIP, port)) {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Connected to server!");
    client.println("Hello from Pico Client!");
  } else {
    Serial.println("Failed to connect to server");
  }
}

void loop() {
  if (client.available()) {
    digitalWrite(LED_BUILTIN, HIGH);
    String msg = client.readStringUntil('\n');
    Serial.printf("Server says: %s\n", msg.c_str());
  }
  delay(1000);
}
