
#define BLYNK_TEMPLATE_ID "TMPL5exN8Wgp7"
#define BLYNK_TEMPLATE_NAME "Potentialmeter"
#define BLYNK_AUTH_TOKEN "cBwmpiU1ZDXiDGDgz5Zh1fNADB6TUsH3"


// Use GPIO 2 for built-in LED (GPIO 10 is often used by internal flash)

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Your WiFi credentials
char ssid[] = "Orange-c45e0";
char pass[] = "48LW8yBb";


// Declaring a global variabl for sensor data
int sensorVal; 

// This function creates the timer object. It's part of Blynk library 
BlynkTimer timer; 

void myTimer() 
{
  // This function describes what will happen with each timer tick
  // e.g. writing sensor value to datastream V5
  Blynk.virtualWrite(V5, sensorVal);  
  //Serial.println(sensorVal);
}

void setup()
{
  //Connecting to Blynk Cloud
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass); 
  Serial.begin(115200);
  
  // Setting interval to send data to Blynk Cloud to 1000ms. 
  // It means that data will be sent every second
  timer.setInterval(1000L, myTimer); 
}

void loop()
{
  // Reading sensor from hardware analog pin A0
sensorVal = map(analogRead(A0), 0, 4095, 0, 115);

  Serial.println(sensorVal);

  // Runs all Blynk stuff
  Blynk.run(); 
  
  // runs BlynkTimer
  timer.run(); 
}