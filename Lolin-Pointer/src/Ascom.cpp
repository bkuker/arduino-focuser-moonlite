#include "Ascom.h"

Ascom::Ascom() : server(80), serverTransactionID(0) {
  this->get("/api/v1/telescope/0/name", this->constant("Telescope Bot"));

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.print("404 - ");
    Serial.println(request->url());
    request->send(404, "text/plain", "Not Found");
  });
}

void Ascom::begin() { server.begin(); };

std::function<void(AsyncWebServerRequest *request)> Ascom::error(
    int error, String message) {
  return [error, message, this](AsyncWebServerRequest *request) {
    Serial.print(request->methodToString());
    Serial.print(" ");
    Serial.println(request->url());
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    DynamicJsonDocument doc(1024);
    AsyncWebParameter *cID =
        request->getParam("ClientTransactionID", request->method() == HTTP_PUT);
    doc["ClientTransactionID"] = cID ? cID->value().toInt() : -1;
    doc["ServerTransactionID"] = ++serverTransactionID;
    doc["ErrorNumber"] = error;
    doc["ErrorMessage"] = message;
    serializeJson(doc, *response);
    request->send(response);
  };
}