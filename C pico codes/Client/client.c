// if you dont see anything in the serial monitor try changing ports from COM3

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
    delay(2000);                      // wait for a second
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  if (client.connect(serverIP, port)) {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Connected to server!");
    client.println("Hello from Pico Client!");
  } else {
    Serial.println("Failed to connect to server");
    Serial.println("Retrying to connect...");    
    WiFi.begin(ssid, password);
  }
}

void loop() {
  if (client.available()) {
    digitalWrite(LED_BUILTIN, HIGH);
    String msg = client.readStringUntil('\n');
    Serial.printf("Server says: %s\n", msg.c_str());

    // if (*msg.c_str() == '1') {
    //   digitalWrite(LED_BUILTIN, HIGH);
    // } else {
    //   digitalWrite(LED_BUILTIN, LOW);
    // }
    
  }
  delay(1000);
}
