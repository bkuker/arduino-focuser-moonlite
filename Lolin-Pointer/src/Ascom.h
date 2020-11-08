#pragma once
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <functional>

#include "Error.h"

class Ascom {
 public:
  Ascom();
  void begin();

 private:
  AsyncWebServer server;
  int serverTransactionID;

 protected:
  template <class G>
  void get(const char *path, G g);

  template <class P>
  void put(const char *path, P p);

  template <class G, class P>
  void prop(const char *path, G g, P p);

  template <class F>
  std::function<void(AsyncWebServerRequest *request)> command(F f);

  template <class F>
  std::function<void(AsyncWebServerRequest *request)> function(F f);

  template <class F>
  std::function<void(AsyncWebServerRequest *request)> function(
      const char *paramName, F f);

  template <class F>
  std::function<void(AsyncWebServerRequest *request)> producer(F f);

  template <class F>
  std::function<void(AsyncWebServerRequest *request)> consumer(F f);

  template <class F>
  std::function<void(AsyncWebServerRequest *request)> consumer(
      const char *paramName, F f);

  template <class V>
  std::function<void(AsyncWebServerRequest *request)> constant(V s);

  template <class F>
  std::function<void(AsyncWebServerRequest *request)> alpacaResponse(F f);

  std::function<void(AsyncWebServerRequest *request)> error(int error,
                                                            String message);
};

template <class G>
void Ascom::get(const char *path, G g) {
  server.on(path, HTTP_GET, g);
}

template <class P>
void Ascom::put(const char *path, P p) {
  server.on(path, HTTP_PUT, p);
}

template <class G, class P>
void Ascom::prop(const char *path, G g, P p) {
  get(path, g);
  put(path, p);
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> Ascom::function(F f) {
  return alpacaResponse(
      [f](AsyncWebServerRequest *request, DynamicJsonDocument &doc) {
        doc["Value"] = f(request);
      });
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> Ascom::function(
    const char *p, F f) {
  return alpacaResponse(
      [p, f](AsyncWebServerRequest *request, DynamicJsonDocument &doc) {
        String v = request->getParam(p, request->method() == HTTP_PUT)->value();
        doc["Value"] = f(v);
      });
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> Ascom::command(F f) {
  return alpacaResponse(
      [f](AsyncWebServerRequest *request, DynamicJsonDocument &doc) { f(); });
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> Ascom::producer(F f) {
  return alpacaResponse([f](AsyncWebServerRequest *request,
                            DynamicJsonDocument &doc) { doc["Value"] = f(); });
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> Ascom::consumer(F f) {
  return alpacaResponse([f](AsyncWebServerRequest *request,
                            DynamicJsonDocument &doc) { f(request); });
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> Ascom::consumer(
    const char *p, F f) {
  return alpacaResponse(
      [p, f](AsyncWebServerRequest *request, DynamicJsonDocument &doc) {
        String v = request->getParam(p, request->method() == HTTP_PUT)->value();
        f(v);
      });
}

template <class V>
std::function<void(AsyncWebServerRequest *request)> Ascom::constant(V s) {
  return alpacaResponse([s](AsyncWebServerRequest *request,
                            DynamicJsonDocument &doc) { doc["Value"] = s; });
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> Ascom::alpacaResponse(F f) {
  return [f, this](AsyncWebServerRequest *request) {
    Serial.print(request->methodToString());
    Serial.print(" ");
    Serial.println(request->url());
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    DynamicJsonDocument doc(1024);
    try {
      f(request, doc);
      doc["ErrorNumber"] = 0;
      doc["ErrorMessage"] = "";
    } catch (Error e) {
      doc["ErrorNumber"] = e.getCode();
      doc["ErrorMessage"] = e.getMessage();
    }

    AsyncWebParameter *cID =
        request->getParam("ClientTransactionID", request->method() == HTTP_PUT);
    doc["ClientTransactionID"] = cID ? cID->value().toInt() : -1;
    doc["ServerTransactionID"] = ++serverTransactionID;
    serializeJson(doc, *response);
    request->send(response);
  };
}
