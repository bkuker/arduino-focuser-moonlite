#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Time.h>
#include <WiFi.h>

#include "Error.h"
#include "Pointer.h"
Pointer ppt;

void setupWifi();
void setupServer();

void setup() {
  Serial.begin(9600);
  setupWifi();
  configTime(0, 0, "pool.ntp.org");
  ppt.begin();
}

void loop() { ppt.tick(); }

///////////////////////////////////////////////////////////////////////////////
//        WIFI SERVER SETUP
///////////////////////////////////////////////////////////////////////////////
const char *ssid = "Taco2.4";
const char *password = "SalsaShark";

void setupWifi() {
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected IP: ");
  Serial.println(WiFi.localIP());
}