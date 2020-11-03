#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <AccelStepper.h>

#define MICROSTEPS 16
/* The gear ratio of the stepper to focuser. 3 means 3 stepper rotations to one focuser rotation */
#define GEAR_RATIO 3
/* How many steps per full revolution of the motor itself (not including gearing) */
#define NATIVE_STEPS_PER_REV 200
/* How many pulses of the STEP pin for one revolution of gear shaft */
#define STEPS_PER_REV (NATIVE_STEPS_PER_REV * GEAR_RATIO * MICROSTEPS)
/*How many seconds for one rotation of the output in full step mode*/
#define SECONDS_PER_REV 3
#define MAXSPEED (STEPS_PER_REV / SECONDS_PER_REV)
#define ACCELERATION 500

/*How long wait after motion is stopped to disable stepper */
#define SETTLE_MS 500

/* Stepper pins */
#define ENABLE_PIN 18
//#define MS1 27
//#define MS2 26
//#define MS3 25
//#define RESET_PIN 17 //Optional
//#define SLEEP_PIN 16
#define STEP_PIN 17
#define DIR_PIN 16

void setupWifi();
void setupServer();
void setupStepper();

void setup()
{
  Serial.begin(9600);
  setupWifi();
  setupServer();
  setupStepper();
}

///////////////////////////////////////////////////////////////////////////////
//        HARDWARE SETUP
///////////////////////////////////////////////////////////////////////////////
//Internal State
long millisLastMove = 0;
long millisLastPrint = 0;
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

void setupStepper()
{
  stepper.setMaxSpeed(MAXSPEED);
  stepper.setAcceleration(ACCELERATION);
  stepper.setEnablePin(ENABLE_PIN);
  stepper.disableOutputs();
  stepper.setPinsInverted(true, false, true);
  millisLastMove = millis();

#ifdef MS1
  pinMode(MS1, OUTPUT);
  digitalWrite(MS1, HIGH);
#endif
#ifdef MS2
  pinMode(MS2, OUTPUT);
  digitalWrite(MS2, HIGH);
#endif
#ifdef MS3
  pinMode(MS3, OUTPUT);
  digitalWrite(MS3, HIGH);
#endif
#ifdef SLEEP_PIN
  pinMode(SLEEP_PIN, OUTPUT);
  digitalWrite(SLEEP_PIN, HIGH);
#endif

#ifdef RESET_PIN
  //Reset driver and pulse it to align to a whole step
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, LOW);
  delay(10);
  digitalWrite(RESET_PIN, HIGH);
  delay(10);
  stepper.enableOutputs();
  delay(10);
  stepper.disableOutputs();
#endif
}

void loop()
{
  long now = millis();
  //Motion Controll
  if (stepper.distanceToGo())
  {
    if (stepper.run())
    {
      millisLastMove = now;
    }
  }
  else
  {
    // reported on INDI forum that some steppers "stutter" if disableOutputs is done repeatedly
    // over a short interval; hence we only disable the outputs and release the motor some seconds
    // after movement has stopped
    if ((now - millisLastMove) > SETTLE_MS)
    {
      stepper.disableOutputs();
    }
  }

  if ((now - millisLastPrint) > 500)
  {
    millisLastPrint = now;
    Serial.print("Position ");
    Serial.println(stepper.currentPosition() / MICROSTEPS);
  }
}

///////////////////////////////////////////////////////////////////////////////
//        WIFI SERVER SETUP
///////////////////////////////////////////////////////////////////////////////
const char *ssid = "Taco2.4";
const char *password = "SalsaShark";

void setupWifi()
{
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
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

void setupServer()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/api/v1/focuser/0/name");
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.print("404 - ");
    Serial.println(request->url());
    request->send(404, "text/plain", "Not Found");
  });

  //Basic Info
  server.on("/api/v1/focuser/0/name", HTTP_GET, constant("FocusBot"));
  server.on("/api/v1/focuser/0/description", HTTP_GET, constant("ESP32 Alpaca Focuser"));
  server.on("/api/v1/focuser/0/driverinfo", HTTP_GET, constant("focuser"));
  server.on("/api/v1/focuser/0/driverversion", HTTP_GET, constant("0.1"));
  server.on("/api/v1/focuser/0/interfaceversion", HTTP_GET, constant(1));

  //Connect
  server.on("/api/v1/focuser/0/connected", HTTP_GET, producer([]() { return connected; }));
  server.on("/api/v1/focuser/0/connected", HTTP_PUT, function([](AsyncWebServerRequest *request) {
              return connected = request->getParam("Connected", true)->value().equalsIgnoreCase("true");
              Serial.print("Set connected ");
              Serial.println(connected);
            }));

  //Focuser
  server.on("/api/v1/focuser/0/absolute", HTTP_GET, constant(true));
  server.on("/api/v1/focuser/0/ismoving", HTTP_GET, producer([]() { return stepper.isRunning(); }));
  server.on("/api/v1/focuser/0/maxincrement", HTTP_GET, constant(1000));
  server.on("/api/v1/focuser/0/maxstep", HTTP_GET, constant(10000));
  server.on("/api/v1/focuser/0/position", HTTP_GET, producer([]() { return stepper.currentPosition(); }));
  server.on("/api/v1/focuser/0/stepsize", HTTP_GET, constant(100));
  server.on("/api/v1/focuser/0/tempcomp", HTTP_GET, constant(false));
  server.on("/api/v1/focuser/0/tempcomp", HTTP_PUT, constant(false));
  server.on("/api/v1/focuser/0/tempcompavailable", HTTP_GET, constant(false));
  server.on("/api/v1/focuser/0/temperature", HTTP_GET, constant(-42));

  server.on("/api/v1/focuser/0/halt", HTTP_PUT, consumer([](AsyncWebServerRequest *request) {
              stepper.moveTo(stepper.currentPosition());
              stepper.setMaxSpeed(1);
              stepper.disableOutputs();
              Serial.println("Halted");
            }));

  server.on("/api/v1/focuser/0/move", HTTP_PUT, consumer([](AsyncWebServerRequest *request) {
              stepper.setMaxSpeed(MAXSPEED);
              stepper.enableOutputs();
              stepper.moveTo(request->getParam("Position", true)->value().toInt() * MICROSTEPS);
              Serial.print("Moving to ");
              Serial.println(stepper.targetPosition() / MICROSTEPS);
            }));

  server.begin();
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> function(F f)
{
  return alpacaResponse([f](AsyncWebServerRequest *request, DynamicJsonDocument &doc) {
    doc["Value"] = f(request);
  });
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> producer(F f)
{
  return alpacaResponse([f](AsyncWebServerRequest *request, DynamicJsonDocument &doc) {
    doc["Value"] = f();
  });
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> consumer(F f)
{
  return alpacaResponse([f](AsyncWebServerRequest *request, DynamicJsonDocument &doc) {
    f(request);
  });
}

template <class V>
std::function<void(AsyncWebServerRequest *request)> constant(V s)
{
  return alpacaResponse([s](AsyncWebServerRequest *request, DynamicJsonDocument &doc) {
    doc["Value"] = s;
  });
}

template <class F>
std::function<void(AsyncWebServerRequest *request)> alpacaResponse(F f)
{
  return [f](AsyncWebServerRequest *request) {
    Serial.println(request->url());
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument doc(1024);
    f(request, doc);
    doc["ClientTransactionID"] = request->getParam("ClientTransactionID", request->method() == HTTP_PUT)->value().toInt();
    doc["ServerTransactionID"] = ++serverTransactionID;
    doc["ErrorNumber"] = 0;
    doc["ErrorMessage"] = "";
    serializeJson(doc, *response);
    request->send(response);
  };
}