#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <husarnet.h>        
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>      // Required for ESP32 hardware PWM compatibility

// WiFi Configurations
#define WIFI_SSID "mnet"
#define WIFI_PASS "mnet2024"

// Husarnet Credentials
#define CLIENT_HOSTNAME "esp32-oled-client"
#define JOIN_CODE "malik.019@://hotmail.com" 

// TARGET SERVER HOSTNAME
const char* targetHost = "fc94:ef44:6d10:9770:44fb:e7af:4dcf:ba61"; 

// Define screen dimensions for the 0.66" OLED
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 48

// Hardware Pins
#define SERVO_PIN 13         // Safe, standard GPIO on ESP-WROOM-32

// Declaration for SSD1306 display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

HusarnetClient husarnet;
Servo myServo;               // Create instances for standard 180 servo

// Global variables to store telemetry values
String dispTemp = "--.-";
String dispHum  = "--";

// UI layout for the 64x48 OLED and Serial Monitor
void updateDisplay(String temp, String hum) {
  Serial.println("\n=== LIVE TELEMETRY FROM SERVER ===");
  Serial.printf("Temperature: %s °C\n", temp.c_str());
  Serial.printf("Humidity:    %s %%\n", hum.c_str());
  Serial.println("==================================\n");

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1); 

  display.setCursor(0, 10); 
  display.print(F("T:"));
  display.print(temp); 
  display.write(247); 
  display.print(F("C")); 

  display.setCursor(0, 26); 
  display.print(F("H:"));
  display.print(hum);
  display.print(F("%"));

  display.display();
}

// Convert temperature values to strict 180-degree motion profiles and stop humming
void moveServoToTemperature(String tempStr) {
  float tempVal = tempStr.toFloat();
  
  if (tempVal == 0.0 && tempStr != "0" && tempStr != "0.0") {
    return; // Ignore invalid conversions or raw startup strings
  }

  // Define expected real-world temperature bounds
  float minTemp = 15.0; 
  float maxTemp = 35.0; 

  int targetAngle = map(constrain(tempVal, minTemp, maxTemp), minTemp, maxTemp, 0, 180);
  
  // Re-attach the pin to establish communication
  if (!myServo.attached()) {
    myServo.attach(SERVO_PIN, 500, 2400); 
  }

  myServo.write(targetAngle);
  Serial.printf("[Servo] Moving to target Angle: %d° for Temp: %.1f°C\n", targetAngle, tempVal);
  
  // Give the motor physical time to reach its destination (500ms is standard for 180 deg)
  delay(500); 
  
  // Detach the pin. This completely cuts the PWM signal and stops the humming/buzzing.
  myServo.detach(); 
  Serial.println("[Servo] Detached to prevent humming.");
}

void setup() {
  Serial.begin(115200);
  delay(1500); 

  // Allocate hardware PWM timers explicitly for the ESP32
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  myServo.setPeriodHertz(50); // Standard 50Hz frequency required by analog/digital servos
  
  // We leave it detached initially until the first valid data frame comes in
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); 
  }
  
  display.clearDisplay();
  display.setTextSize(1); 
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println(F("BOOTING..."));
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  Serial.print("Connecting to local network");
  display.setCursor(0, 22);
  display.print(F("WiFi Link:"));
  display.display();

  int timeoutCounter = 0;
  while (WiFi.status() != WL_CONNECTED && timeoutCounter < 40) {
    delay(500);
    Serial.print(".");
    display.print(F("."));
    display.display();
    timeoutCounter++;
  }

  if(WiFi.status() != WL_CONNECTED) {
    display.clearDisplay();
    display.setCursor(0, 18);
    display.print(F("No WiFi!"));
    display.display();
    delay(3000);
    ESP.restart(); 
  } else {
    Serial.println("\nWiFi Connected locally!");
    Serial.printf("Client Local IP: %s\n", WiFi.localIP().toString().c_str());
  }

  display.clearDisplay();
  display.setCursor(0, 10);
  display.println(F("HUSARNET.."));
  display.display();

  husarnet.join(CLIENT_HOSTNAME, JOIN_CODE);

  while(!husarnet.isJoined()) {
    Serial.println("Waiting for Husarnet network...");
    delay(1000);
  }
  Serial.println("Husarnet network joined");
  
  updateDisplay(dispTemp, dispHum);
}

void loop() {
  WiFiClient client;
  Serial.printf("Connecting to server host: %s...\n", targetHost);
  
  if (!client.connect(targetHost, 80)) {
    Serial.println("Connection to server failed. Retrying in 5 seconds...");
    delay(5000);
    return;
  }

  client.print("GET /data HTTP/1.1\r\n"
               "Host: ");
  client.print(targetHost);
  client.print("\r\n"
               "Connection: close\r\n\r\n");

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 3000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      delay(5000);
      return;
    }
  }

  String response = "";
  while(client.available()) {
    response += client.readString();
  }
  
  client.stop(); 
  
  int bodyIndex = response.indexOf("\r\n\r\n");
  if (bodyIndex != -1) {
    String jsonBody = response.substring(bodyIndex + 4);
    
    int tempPos = jsonBody.indexOf("\"temp\":");
    int humPos = jsonBody.indexOf("\"hum\":");
    
    if (tempPos != -1 && humPos != -1) {
      int tempStart = tempPos + 7;
      int tempEnd = jsonBody.indexOf(",", tempStart);
      dispTemp = jsonBody.substring(tempStart, tempEnd);
      
      int humStart = humPos + 6;
      int humEnd = jsonBody.indexOf("}", humStart);
      dispHum = jsonBody.substring(humStart, humEnd);
      
      updateDisplay(dispTemp, dispHum);
      moveServoToTemperature(dispTemp); 
    }
  } else {
    Serial.println("Invalid payload container received.");
  }

  delay(5000); 
}
