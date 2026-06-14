#include <Arduino.h>
#include <WiFi.h>
#include "lwip/sockets.h"
#include <Bonezegei_DHT11.h>

#define WIFI_SSID "mnet"
#define WIFI_PASS "mnet2024"

int server_fd = -1;
Bonezegei_DHT11 dht(13); 

float globalTemp = 0.0;
int globalHum = 0;
unsigned long lastSensorRead = 0;
const unsigned long sensorInterval = 2000; 

void setup() {
  Serial.begin(115200);
  delay(1500); 
  
  dht.begin();
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  Serial.print("Server IP: ");
  Serial.println(WiFi.localIP());

  struct sockaddr_in server_addr;
  server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  fcntl(server_fd, F_SETFL, O_NONBLOCK);

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(80);
  server_addr.sin_addr.s_addr = INADDR_ANY; 

  bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  listen(server_fd, 4);
}

void loop() {
  // 1. Non-blocking sensor update
  unsigned long currentMillis = millis();
  if (currentMillis - lastSensorRead >= sensorInterval) {
    lastSensorRead = currentMillis;
    if (dht.getData()) {
      globalTemp = dht.getTemperature();
      globalHum = dht.getHumidity();
    }
  }

  // 2. Non-blocking network connection handling
  struct sockaddr_in source_addr;
  socklen_t addr_len = sizeof(source_addr);
  int client_fd = accept(server_fd, (struct sockaddr *)&source_addr, &addr_len);
  
  if (client_fd < 0) return; 

  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  char rx_buffer[512];
  memset(rx_buffer, 0, sizeof(rx_buffer));
  int len = recv(client_fd, rx_buffer, sizeof(rx_buffer) - 1, 0);
  
  if (len > 0) {
    String request = String(rx_buffer);

    // FIX 1: Strict JSON Data Priority Route
    if (request.indexOf("/data") != -1) {
      String jsonPayload = "{\"temp\":" + String(globalTemp, 1) + ",\"hum\":" + String(globalHum) + "}";
      String response = "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Connection: close\r\n\r\n" + jsonPayload;
      send(client_fd, response.c_str(), response.length(), 0);
      Serial.println("[API Served] Successfully dispatched raw JSON telemetry.");
    } 
    // FIX 2: Strict HTML browser route configuration match
    else if (request.indexOf("GET /") != -1) {
      String htmlBody = 
        "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
        "<style>body{font-family:sans-serif;text-align:center;background:#f4f6f9;color:#333;margin-top:50px;}"
        ".card{background:white;display:inline-block;padding:30px;border-radius:10px;box-shadow:0 4px 8px rgba(0,0,0,0.1);}"
        "h1{color:#0066cc;} .val{font-size:2.5em;font-weight:bold;color:#ff5722;}</style>"
        "<script>"
        "setInterval(function(){"
          "fetch('/data').then(r=>r.json()).then(d=>{"
            "document.getElementById('t').innerText=d.temp+'°C';"
            "document.getElementById('h').innerText=d.hum+'%';"
          "});"
        "},2000);" 
        "</script></head><body>"
        "<div class='card'><h1>ESP32-CAM Live Telemetry</h1>"
        "<p>Temperature: <span id='t' class='val'>--°C</span></p>"
        "<p>Humidity: <span id='h' class='val'>--%</span></p>"
        "</div></body></html>\r\n";

      String response = "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: " + String(htmlBody.length()) + "\r\n"
                        "Connection: close\r\n\r\n" + htmlBody;
      send(client_fd, response.c_str(), response.length(), 0);
    }
  }
  close(client_fd);
}