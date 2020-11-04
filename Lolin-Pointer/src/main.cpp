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
  phy.setAltAz(0,0);
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

//Alpaca State
int serverTransactionID = 0;
boolean connected = false;

//Parking
boolean parked = false;
int parkAlt = 0;
int parkAz = 0;

template <class F>
std::function<void(AsyncWebServerRequest *request)> alpacaResponse(F f);
template <class F>
std::function<void(AsyncWebServerRequest *request)> command(F f);
template <class F>
std::function<void(AsyncWebServerRequest *request)> function(F f);
template <class F>
std::function<void(AsyncWebServerRequest *request)> producer(F f);
template <class F>
std::function<void(AsyncWebServerRequest *request)> consumer(F f);
template <class V>
std::function<void(AsyncWebServerRequest *request)> constant(V s);

std::function<void(AsyncWebServerRequest *request)> error(int error,
                                                          String message);

std::function<void(AsyncWebServerRequest *request)> unimplemented =
    error(1024, "Property or Method Not Implemented");

void setupServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/api/v1/api/v1/telescope/0/name");
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.print("404 - ");
    Serial.println(request->url());
    request->send(404, "text/plain", "Not Found");
  });

  ///////// Common

  // Invokes the named device-specific action.
  server.on("/api/v1/telescope/0/action", HTTP_PUT, unimplemented);

  // Transmits an arbitrary string to the device
  server.on("/api/v1/telescope/0/commandblind", HTTP_PUT, unimplemented);

  // Transmits an arbitrary string to the device and returns a boolean value
  // from the device.
  server.on("/api/v1/telescope/0/commandbool", HTTP_PUT, unimplemented);

  // Transmits an arbitrary string to the device and returns a string value from
  // the device.
  server.on("/api/v1/telescope/0/commandstring", HTTP_PUT, unimplemented);

  // Retrieves the connected state of the device
  server.on("/api/v1/telescope/0/connected", HTTP_GET, producer([]() { return connected; }));

  // Sets the connected state of the device
  server.on("/api/v1/telescope/0/connected", HTTP_PUT, function([](AsyncWebServerRequest *request) {
              return connected = request->getParam("Connected", true)->value().equalsIgnoreCase("true");
              Serial.print("Set connected ");
              Serial.println(connected);
            }));


  // Device description
  server.on("/api/v1/telescope/0/description", HTTP_GET, constant("ESP32 Alpaca Telescope"));

  // Device driver description
  server.on("/api/v1/telescope/0/driverinfo", HTTP_GET, constant("telescope"));

  // Driver Version
  server.on("/api/v1/telescope/0/driverversion", HTTP_GET, constant("0.1"));

  // The ASCOM Device interface version number that this device supports.
  server.on("/api/v1/telescope/0/interfaceversion", HTTP_GET, constant(2));

  // Device name
  server.on("/api/v1/telescope/0/name", HTTP_GET, constant("Telescope Bot"));

  // Returns the list of action names supported by this driver.
  server.on("/api/v1/telescope/0/supportedactions", HTTP_GET, unimplemented);

  //////// TELESCOPE

  //Home
  // Indicates whether the mount can find the home position.
  server.on("/api/v1/telescope/0/canfindhome", HTTP_GET, constant(false));
  // Indicates whether the mount is at the home position.
  server.on("/api/v1/telescope/0/athome", HTTP_GET, producer([]() {
    return phy.getAlt() == 0 && phy.getAz() == 0;
  }));
  // Moves the mount to the "home" position.
  server.on("/api/v1/telescope/0/findhome", HTTP_PUT, unimplemented);


  //Park
  // Indicates whether the telescope can be parked.
  server.on("/api/v1/telescope/0/canpark", HTTP_GET, constant(true));
  // Indicates whether the telescope park position can be set.
  server.on("/api/v1/telescope/0/cansetpark", HTTP_GET, constant(true));
  // Indicates whether the telescope can unpark
  server.on("/api/v1/telescope/0/canunpark", HTTP_GET, constant(true));
  // Indicates whether the telescope is at the park position.
  server.on("/api/v1/telescope/0/atpark", HTTP_GET, producer([]() {
    return parked && phy.getAlt() == parkAlt && phy.getAz() == parkAz;
  }));
  // Park the mount
  server.on("/api/v1/telescope/0/park", HTTP_PUT, command([](){
    phy.setAltAz(0,0);
    parked = true;
  }));
  // Sets the telescope's park position
  server.on("/api/v1/telescope/0/setpark", HTTP_PUT, command([](){
    parkAlt = phy.getAz();
    parkAz = phy.getAz();
  }));
  // Unparks the mount.
  server.on("/api/v1/telescope/0/unpark", HTTP_PUT, command([](){
    parked = false;
  }));

  //Optics
  // Returns the telescope's aperture.
  server.on("/api/v1/telescope/0/aperturearea", HTTP_GET, unimplemented);
  // Returns the telescope's effective aperture.
  server.on("/api/v1/telescope/0/aperturediameter", HTTP_GET, unimplemented);
  // Returns the telescope's focal length in meters.
  server.on("/api/v1/telescope/0/focallength", HTTP_GET, unimplemented);
  // Indicates whether atmospheric refraction is applied to coordinates.
  server.on("/api/v1/telescope/0/doesrefraction", HTTP_GET, unimplemented);
  // Determines whether atmospheric refraction is applied to coordinates.
  server.on("/api/v1/telescope/0/doesrefraction", HTTP_PUT, unimplemented);


  //Tracking
  // Returns the telescope's declination tracking rate.
  server.on("/api/v1/telescope/0/declinationrate", HTTP_GET, constant(0));
  // Sets the telescope's declination tracking rate.
  server.on("/api/v1/telescope/0/declinationrate", HTTP_PUT, unimplemented);
  // Returns the telescope's right ascension tracking rate.
  server.on("/api/v1/telescope/0/rightascensionrate", HTTP_GET, constant(0));
  // Sets the telescope's right ascension tracking rate.
  server.on("/api/v1/telescope/0/rightascensionrate", HTTP_PUT, unimplemented);
  // Indicates whether the Tracking property can be changed.
  server.on("/api/v1/telescope/0/cansettracking", HTTP_GET, constant(false));
  // Indicates whether the telescope is tracking.
  server.on("/api/v1/telescope/0/tracking", HTTP_GET, unimplemented);
  // Enables or disables telescope tracking.
  server.on("/api/v1/telescope/0/tracking", HTTP_PUT, unimplemented);
  // Returns the current tracking rate.
  server.on("/api/v1/telescope/0/trackingrate", HTTP_GET, unimplemented);
  // Sets the mount's tracking rate.
  server.on("/api/v1/telescope/0/trackingrate", HTTP_PUT, unimplemented);
  // Returns a collection of supported DriveRates values.
  server.on("/api/v1/telescope/0/trackingrates", HTTP_GET, unimplemented);


  //Guiding
  // Indicates whether the telescope can be pulse guided.
  server.on("/api/v1/telescope/0/canpulseguide", HTTP_GET, constant(false));
  // Indicates whether the DeclinationRate property can be changed.
  server.on("/api/v1/telescope/0/cansetguiderates", HTTP_GET, constant(false));
  // Returns the current Declination rate offset for telescope guiding
  server.on("/api/v1/telescope/0/guideratedeclination", HTTP_GET, unimplemented);
  // Sets the current Declination rate offset for telescope guiding.
  server.on("/api/v1/telescope/0/guideratedeclination", HTTP_PUT, unimplemented);
  // Returns the current RightAscension rate offset for telescope guiding
  server.on("/api/v1/telescope/0/guideraterightascension", HTTP_GET, unimplemented);
  // Sets the current RightAscension rate offset for telescope guiding.
  server.on("/api/v1/telescope/0/guideraterightascension", HTTP_PUT, unimplemented);
  // Moves the scope in the given direction for the given time.
  server.on("/api/v1/telescope/0/pulseguide", HTTP_PUT, unimplemented);
  // Indicates whether the telescope is currently executing a PulseGuide command
  server.on("/api/v1/telescope/0/ispulseguiding", HTTP_GET, unimplemented);

  //Sync
  // Syncs to the given local horizontal coordinates.
  server.on("/api/v1/telescope/0/synctoaltaz", HTTP_PUT, unimplemented);
  // Syncs to the given equatorial coordinates.
  server.on("/api/v1/telescope/0/synctocoordinates", HTTP_PUT, unimplemented);
  // Syncs to the TargetRightAscension and TargetDeclination coordinates.
  server.on("/api/v1/telescope/0/synctotarget", HTTP_PUT, unimplemented);

  //Site
  // Returns the observing site's elevation above mean sea level.
  server.on("/api/v1/telescope/0/siteelevation", HTTP_GET, unimplemented);
  // Sets the observing site's elevation above mean sea level.
  server.on("/api/v1/telescope/0/siteelevation", HTTP_PUT, unimplemented);
  // Returns the observing site's latitude.
  server.on("/api/v1/telescope/0/sitelatitude", HTTP_GET, unimplemented);
  // Sets the observing site's latitude.
  server.on("/api/v1/telescope/0/sitelatitude", HTTP_PUT, unimplemented);
  // Returns the observing site's longitude.
  server.on("/api/v1/telescope/0/sitelongitude", HTTP_GET, unimplemented);
  // Sets the observing site's longitude.
  server.on("/api/v1/telescope/0/sitelongitude", HTTP_PUT, unimplemented);

  //Pier
  // Indicates whether the telescope SideOfPier can be set.
  server.on("/api/v1/telescope/0/cansetpierside", HTTP_GET, constant(false));
  // Returns the mount's pointing state.
  server.on("/api/v1/telescope/0/sideofpier", HTTP_GET, unimplemented);
  // Sets the mount's pointing state.
  server.on("/api/v1/telescope/0/sideofpier", HTTP_PUT, unimplemented);
  // Predicts the pointing state after a German equatorial mount slews to given
  // coordinates.
  server.on("/api/v1/telescope/0/destinationsideofpier", HTTP_GET, unimplemented);


  //Alignment Mode
  // Returns the current mount alignment mode
  server.on("/api/v1/telescope/0/alignmentmode", HTTP_GET, constant(0));
  // Returns the current equatorial coordinate system used by this telescope.
  server.on("/api/v1/telescope/0/equatorialsystem", HTTP_GET, constant(1));


  //Slewing

  // Indicates whether the telescope can slew synchronously.
  server.on("/api/v1/telescope/0/canslew", HTTP_GET, constant(false));
  // Indicates whether the telescope can slew asynchronously.
  server.on("/api/v1/telescope/0/canslewasync", HTTP_GET, constant(false));

  //////Slew - Equitorial
  // Synchronously slew to the given equatorial coordinates.
  server.on("/api/v1/telescope/0/slewtocoordinates", HTTP_PUT, unimplemented);
  // Asynchronously slew to the given equatorial coordinates.
  server.on("/api/v1/telescope/0/slewtocoordinatesasync", HTTP_PUT, unimplemented);




  //////Slew - RA /Dec
  // Returns the current target declination.
  server.on("/api/v1/telescope/0/targetdeclination", HTTP_GET, unimplemented);
  // Returns the current target right ascension.
  server.on("/api/v1/telescope/0/targetrightascension", HTTP_GET, unimplemented);
  // Sets the target declination of a slew or sync.
  server.on("/api/v1/telescope/0/targetdeclination", HTTP_PUT, unimplemented);
  // Sets the target right ascension of a slew or sync.
  server.on("/api/v1/telescope/0/targetrightascension", HTTP_PUT, unimplemented);
  // Synchronously slew to the TargetRightAscension and TargetDeclination
  server.on("/api/v1/telescope/0/slewtotarget", HTTP_PUT, unimplemented);
  // Asynchronously slew to the TargetRightAscension and TargetDeclination
  server.on("/api/v1/telescope/0/slewtotargetasync", HTTP_PUT, unimplemented);

  //////Slew - Alt / Az
  // Indicates whether the telescope can slew synchronously to AltAz
  server.on("/api/v1/telescope/0/canslewaltaz", HTTP_GET, constant(true));
  // Indicates whether the telescope can slew asynchronously to AltAz
  server.on("/api/v1/telescope/0/canslewaltazasync", HTTP_GET, constant(true));
  // Returns the mount's altitude above the horizon.
  server.on("/api/v1/telescope/0/altitude", HTTP_GET, producer([](){return phy.getAlt();}));
  // Returns the mount's azimuth.
  server.on("/api/v1/telescope/0/azimuth", HTTP_GET, producer([](){return phy.getAlt();}));
  // Synchronously slew to the given local horizontal coordinates.
  server.on("/api/v1/telescope/0/slewtoaltaz", HTTP_PUT, unimplemented);
  // Asynchronously slew to the given local horizontal coordinates.
  server.on("/api/v1/telescope/0/slewtoaltazasync", HTTP_PUT, unimplemented);


  // Sync
  // Indicates whether the telescope can sync to equatorial coordinates.
  server.on("/api/v1/telescope/0/cansync", HTTP_GET, constant(false));
  // Indicates whether the telescope can sync to local horizontal coordinates.
  server.on("/api/v1/telescope/0/cansyncaltaz", HTTP_GET, constant(false));

  //Rates
  // Indicates whether the DeclinationRate property can be changed.
  server.on("/api/v1/telescope/0/cansetdeclinationrate", HTTP_GET, constant(false));
  // Indicates whether the RightAscensionRate property can be changed.
  server.on("/api/v1/telescope/0/cansetrightascensionrate", HTTP_GET, constant(false));

  //Motion
  // Indicates whether the telescope is currently slewing.
  server.on("/api/v1/telescope/0/slewing", HTTP_GET, unimplemented);
  // Returns the post-slew settling time.
  server.on("/api/v1/telescope/0/slewsettletime", HTTP_GET, unimplemented);
  // Sets the post-slew settling time.
  server.on("/api/v1/telescope/0/slewsettletime", HTTP_PUT, unimplemented);
  // Immediatley stops a slew in progress.
  server.on("/api/v1/telescope/0/abortslew", HTTP_PUT, unimplemented);

  //Current Position
  // Returns the mount's declination.
  server.on("/api/v1/telescope/0/declination", HTTP_GET, unimplemented);
  // Returns the mount's right ascension coordinate.
  server.on("/api/v1/telescope/0/rightascension", HTTP_GET, unimplemented);

  //Time
  // Returns the local apparent sidereal time.
  server.on("/api/v1/telescope/0/siderealtime", HTTP_GET, unimplemented);
  // Returns the UTC date/time of the telescope's internal clock.
  server.on("/api/v1/telescope/0/utcdate", HTTP_GET, unimplemented);
  // Sets the UTC date/time of the telescope's internal clock.
  server.on("/api/v1/telescope/0/utcdate", HTTP_PUT, unimplemented);








  // Returns the rates at which the telescope may be moved about the specified
  // axis.
  server.on("/api/v1/telescope/0/axisrates", HTTP_GET, unimplemented);

  // Indicates whether the telescope can move the requested axis.
  server.on("/api/v1/telescope/0/canmoveaxis", HTTP_GET, unimplemented);



  // Moves a telescope axis at the given rate.
  server.on("/api/v1/telescope/0/moveaxis", HTTP_PUT, unimplemented);












  server.begin();
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> function(F f) {
  return alpacaResponse(
      [f](AsyncWebServerRequest *request, DynamicJsonDocument &doc) {
        doc["Value"] = f(request);
      });
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> command(F f) {
  return alpacaResponse([f](AsyncWebServerRequest *request,
                            DynamicJsonDocument &doc) { f(); });
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

std::function<void(AsyncWebServerRequest *request)> error(int error,
                                                          String message) {
  return [error, message](AsyncWebServerRequest *request) {
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