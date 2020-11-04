#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "Phy.h"

void setupWifi();
void setupServer();

Phy phy;

void setup() {
  Serial.begin(9600);
  setupWifi();
  setupServer();
  phy.altCalCircle();
  phy.setAltAz(45, 90);
}

void loop() {
  phy.tick();
  delay(3);
}

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

///////////////////////////////////////////////////////////////////////////////
//        WEB SERVER SETUP
///////////////////////////////////////////////////////////////////////////////
AsyncWebServer server(80);
int serverTransactionID = 0;
boolean connected = false;

template <class F>
std::function<void(AsyncWebServerRequest *request)> alpacaResponse(F f);
template <class F>
std::function<void(AsyncWebServerRequest *request)> function(F f);
template <class F>
std::function<void(AsyncWebServerRequest *request)> producer(F f);
template <class F>
std::function<void(AsyncWebServerRequest *request)> consumer(F f);
template <class V>
std::function<void(AsyncWebServerRequest *request)> constant(V s);

void setupServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/api/v1/telescope/0/name");
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.print("404 - ");
    Serial.println(request->url());
    request->send(404, "text/plain", "Not Found");
  });

  // Basic Info
  server.on("/api/v1/telescope/0/name", HTTP_GET, constant("PointerBot"));
  server.begin();

  // Returns the current mount alignment mode
  server.on("/telescope/0/alignmentmode", HTTP_GET, constant(0));

  // Returns the mount's altitude above the horizon.
  server.on("/telescope/0/altitude", HTTP_GET, constant(0));

  // Returns the telescope's aperture.
  server.on("/telescope/0/aperturearea", HTTP_GET, constant(0));

  // Returns the telescope's effective aperture.
  server.on("/telescope/0/aperturediameter", HTTP_GET,
            constant(0));

  // Indicates whether the mount is at the home position.
  server.on("/telescope/0/athome", HTTP_GET, constant(0));

  // Indicates whether the telescope is at the park position.
  server.on("/telescope/0/atpark", HTTP_GET, constant(0));

  // Returns the mount's azimuth.
  server.on("/telescope/0/azimuth", HTTP_GET, constant(0));

  // Indicates whether the mount can find the home position.
  server.on("/telescope/0/canfindhome", HTTP_GET, constant(0));

  // Indicates whether the telescope can be parked.
  server.on("/telescope/0/canpark", HTTP_GET, constant(0));

  // Indicates whether the telescope can be pulse guided.
  server.on("/telescope/0/canpulseguide", HTTP_GET, constant(0));

  // Indicates whether the DeclinationRate property can be changed.
  server.on("/telescope/0/cansetdeclinationrate", HTTP_GET,
            constant(0));

  // Indicates whether the DeclinationRate property can be changed.
  server.on("/telescope/0/cansetguiderates", HTTP_GET,
            constant(0));

  // Indicates whether the telescope park position can be set.
  server.on("/telescope/0/cansetpark", HTTP_GET, constant(0));

  // Indicates whether the telescope SideOfPier can be set.
  server.on("/telescope/0/cansetpierside", HTTP_GET, constant(0));

  // Indicates whether the RightAscensionRate property can be changed.
  server.on("/telescope/0/cansetrightascensionrate", HTTP_GET,
            constant(0));

  // Indicates whether the Tracking property can be changed.
  server.on("/telescope/0/cansettracking", HTTP_GET, constant(0));

  // Indicates whether the telescope can slew synchronously.
  server.on("/telescope/0/canslew", HTTP_GET, constant(0));

  // Indicates whether the telescope can slew synchronously to AltAz
  // coordinates.
  server.on("/telescope/0/canslewaltaz", HTTP_GET, constant(0));

  // Indicates whether the telescope can slew asynchronously to AltAz
  // coordinates.
  server.on("/telescope/0/canslewaltazasync", HTTP_GET,
            constant(0));

  // Indicates whether the telescope can slew asynchronously.
  server.on("/telescope/0/canslewasync", HTTP_GET, constant(0));

  // Indicates whether the telescope can sync to equatorial coordinates.
  server.on("/telescope/0/cansync", HTTP_GET, constant(0));

  // Indicates whether the telescope can sync to local horizontal coordinates.
  server.on("/telescope/0/cansyncaltaz", HTTP_GET, constant(0));

  // Returns the mount's declination.
  server.on("/telescope/0/declination", HTTP_GET, constant(0));

  // Returns the telescope's declination tracking rate.
  server.on("/telescope/0/declinationrate", HTTP_GET,
            constant(0));

  // Sets the telescope's declination tracking rate.
  server.on("/telescope/0/declinationrate", HTTP_PUT,
            constant(0));

  // Indicates whether atmospheric refraction is applied to coordinates.
  server.on("/telescope/0/doesrefraction", HTTP_GET, constant(0));

  // Determines whether atmospheric refraction is applied to coordinates.
  server.on("/telescope/0/doesrefraction", HTTP_PUT, constant(0));

  // Returns the current equatorial coordinate system used by this telescope.
  server.on("/telescope/0/equatorialsystem", HTTP_GET,
            constant(0));

  // Returns the telescope's focal length in meters.
  server.on("/telescope/0/focallength", HTTP_GET, constant(0));

  // Returns the current Declination rate offset for telescope guiding
  server.on("/telescope/0/guideratedeclination", HTTP_GET,
            constant(0));

  // Sets the current Declination rate offset for telescope guiding.
  server.on("/telescope/0/guideratedeclination", HTTP_PUT,
            constant(0));

  // Returns the current RightAscension rate offset for telescope guiding
  server.on("/telescope/0/guideraterightascension", HTTP_GET,
            constant(0));

  // Sets the current RightAscension rate offset for telescope guiding.
  server.on("/telescope/0/guideraterightascension", HTTP_PUT,
            constant(0));

  // Indicates whether the telescope is currently executing a PulseGuide command
  server.on("/telescope/0/ispulseguiding", HTTP_GET, constant(0));

  // Returns the mount's right ascension coordinate.
  server.on("/telescope/0/rightascension", HTTP_GET, constant(0));

  // Returns the telescope's right ascension tracking rate.
  server.on("/telescope/0/rightascensionrate", HTTP_GET,
            constant(0));

  // Sets the telescope's right ascension tracking rate.
  server.on("/telescope/0/rightascensionrate", HTTP_PUT,
            constant(0));

  // Returns the mount's pointing state.
  server.on("/telescope/0/sideofpier", HTTP_GET, constant(0));

  // Sets the mount's pointing state.
  server.on("/telescope/0/sideofpier", HTTP_PUT, constant(0));

  // Returns the local apparent sidereal time.
  server.on("/telescope/0/siderealtime", HTTP_GET, constant(0));

  // Returns the observing site's elevation above mean sea level.
  server.on("/telescope/0/siteelevation", HTTP_GET, constant(0));

  // Sets the observing site's elevation above mean sea level.
  server.on("/telescope/0/siteelevation", HTTP_PUT, constant(0));

  // Returns the observing site's latitude.
  server.on("/telescope/0/sitelatitude", HTTP_GET, constant(0));

  // Sets the observing site's latitude.
  server.on("/telescope/0/sitelatitude", HTTP_PUT, constant(0));

  // Returns the observing site's longitude.
  server.on("/telescope/0/sitelongitude", HTTP_GET, constant(0));

  // Sets the observing site's longitude.
  server.on("/telescope/0/sitelongitude", HTTP_PUT, constant(0));

  // Indicates whether the telescope is currently slewing.
  server.on("/telescope/0/slewing", HTTP_GET, constant(0));

  // Returns the post-slew settling time.
  server.on("/telescope/0/slewsettletime", HTTP_GET, constant(0));

  // Sets the post-slew settling time.
  server.on("/telescope/0/slewsettletime", HTTP_PUT, constant(0));

  // Returns the current target declination.
  server.on("/telescope/0/targetdeclination", HTTP_GET,
            constant(0));

  // Sets the target declination of a slew or sync.
  server.on("/telescope/0/targetdeclination", HTTP_PUT,
            constant(0));

  // Returns the current target right ascension.
  server.on("/telescope/0/targetrightascension", HTTP_GET,
            constant(0));

  // Sets the target right ascension of a slew or sync.
  server.on("/telescope/0/targetrightascension", HTTP_PUT,
            constant(0));

  // Indicates whether the telescope is tracking.
  server.on("/telescope/0/tracking", HTTP_GET, constant(0));

  // Enables or disables telescope tracking.
  server.on("/telescope/0/tracking", HTTP_PUT, constant(0));

  // Returns the current tracking rate.
  server.on("/telescope/0/trackingrate", HTTP_GET, constant(0));

  // Sets the mount's tracking rate.
  server.on("/telescope/0/trackingrate", HTTP_PUT, constant(0));

  // Returns a collection of supported DriveRates values.
  server.on("/telescope/0/trackingrates", HTTP_GET, constant(0));

  // Returns the UTC date/time of the telescope's internal clock.
  server.on("/telescope/0/utcdate", HTTP_GET, constant(0));

  // Sets the UTC date/time of the telescope's internal clock.
  server.on("/telescope/0/utcdate", HTTP_PUT, constant(0));

  // Immediatley stops a slew in progress.
  server.on("/telescope/0/abortslew", HTTP_PUT, constant(0));

  // Returns the rates at which the telescope may be moved about the specified
  // axis.
  server.on("/telescope/0/axisrates", HTTP_GET, constant(0));

  // Indicates whether the telescope can move the requested axis.
  server.on("/telescope/0/canmoveaxis", HTTP_GET, constant(0));

  // Predicts the pointing state after a German equatorial mount slews to given
  // coordinates.
  server.on("/telescope/0/destinationsideofpier", HTTP_GET,
            constant(0));

  // Moves the mount to the "home" position.
  server.on("/telescope/0/findhome", HTTP_PUT, constant(0));

  // Moves a telescope axis at the given rate.
  server.on("/telescope/0/moveaxis", HTTP_PUT, constant(0));

  // Park the mount
  server.on("/telescope/0/park", HTTP_PUT, constant(0));

  // Moves the scope in the given direction for the given time.
  server.on("/telescope/0/pulseguide", HTTP_PUT, constant(0));

  // Sets the telescope's park position
  server.on("/telescope/0/setpark", HTTP_PUT, constant(0));

  // Synchronously slew to the given local horizontal coordinates.
  server.on("/telescope/0/slewtoaltaz", HTTP_PUT, constant(0));

  // Asynchronously slew to the given local horizontal coordinates.
  server.on("/telescope/0/slewtoaltazasync", HTTP_PUT,
            constant(0));

  // Synchronously slew to the given equatorial coordinates.
  server.on("/telescope/0/slewtocoordinates", HTTP_PUT,
            constant(0));

  // Asynchronously slew to the given equatorial coordinates.
  server.on("/telescope/0/slewtocoordinatesasync", HTTP_PUT,
            constant(0));

  // Synchronously slew to the TargetRightAscension and TargetDeclination
  // coordinates.
  server.on("/telescope/0/slewtotarget", HTTP_PUT, constant(0));

  // Asynchronously slew to the TargetRightAscension and TargetDeclination
  // coordinates.
  server.on("/telescope/0/slewtotargetasync", HTTP_PUT,
            constant(0));

  // Syncs to the given local horizontal coordinates.
  server.on("/telescope/0/synctoaltaz", HTTP_PUT, constant(0));

  // Syncs to the given equatorial coordinates.
  server.on("/telescope/0/synctocoordinates", HTTP_PUT,
            constant(0));

  // Syncs to the TargetRightAscension and TargetDeclination coordinates.
  server.on("/telescope/0/synctotarget", HTTP_PUT, constant(0));

  // Unparks the mount.
  server.on("/telescope/0/unpark", HTTP_PUT, constant(0));
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> function(F f) {
  return alpacaResponse(
      [f](AsyncWebServerRequest *request, DynamicJsonDocument &doc) {
        doc["Value"] = f(request);
      });
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> producer(F f) {
  return alpacaResponse([f](AsyncWebServerRequest *request,
                            DynamicJsonDocument &doc) { doc["Value"] = f(); });
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> consumer(F f) {
  return alpacaResponse([f](AsyncWebServerRequest *request,
                            DynamicJsonDocument &doc) { f(request); });
}

template <class V>
std::function<void(AsyncWebServerRequest *request)> constant(V s) {
  return alpacaResponse([s](AsyncWebServerRequest *request,
                            DynamicJsonDocument &doc) { doc["Value"] = s; });
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> alpacaResponse(F f) {
  return [f](AsyncWebServerRequest *request) {
    Serial.println(request->url());
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    DynamicJsonDocument doc(1024);
    f(request, doc);
    AsyncWebParameter *cID =
        request->getParam("ClientTransactionID", request->method() == HTTP_PUT);
    doc["ClientTransactionID"] = cID ? cID->value().toInt() : -1;
    doc["ServerTransactionID"] = ++serverTransactionID;
    doc["ErrorNumber"] = 0;
    doc["ErrorMessage"] = "";
    serializeJson(doc, *response);
    request->send(response);
  };
}
