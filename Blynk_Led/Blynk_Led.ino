#define BLYNK_TEMPLATE_ID "TMPL5pyPNEr_b"
#define BLYNK_TEMPLATE_NAME "NetworkBlink"
#define BLYNK_AUTH_TOKEN "M48Pm2mKoycKpG493HZNSSvnO1gZDTPH"


// Use GPIO 2 for built-in LED (GPIO 10 is often used by internal flash)
#define LED_PIN 2 

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Your WiFi credentials
char ssid[] = "Orange-c45e0";
char pass[] = "48LW8yBb";

// This function syncs the board with the server's current button state on startup
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V0); 
}

// This function runs every time you toggle the button in the app
BLYNK_WRITE(V0) {
  int value = param.asInt(); // Get value (0 or 1)
  
  Serial.print("Blynk Command Received! State: ");
  Serial.println(value);
  
  digitalWrite(LED_PIN, value); // Control the LED
}

void setup() {
  // Set Baud Rate to 115200 in your Serial Monitor
  Serial.begin(115200);
  
  pinMode(LED_PIN, OUTPUT);
  
  // Start Blynk connection
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  Blynk.run();
  if (millis() % 5000 == 0) { // Every 5 seconds
     Serial.print("Connection status: ");
     Serial.println(Blynk.connected());
  }
}
