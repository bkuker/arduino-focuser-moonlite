#include "Pointer.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#define VALUE(V) producer([this] { return (V); })
#define URL "/api/v1/telescope/0/"

std::string getCurrentTimeFormatted();

void Pointer::tick(){
  phy.tick();
  delay(1);

  if ( !phy.isMoving() && isTracking && targetRA != -1 && targetDec != -1){
    try {
        double alt, az;
        as.convert(time(NULL), (targetRA/24.f)*360.0f, targetDec, &alt, &az);
        phy.setAltAz(alt, az);
    } catch (Error e){
      Serial.print("Error during tracking: ");
      Serial.println(e.getMessage().c_str());
    }
    delay(1000);
  }
}

Pointer::Pointer()
    :  // Start disconnected
      connected(false),
      // Rutland, Vermont
      as(43.554736, -73.249809),
      ////Parking
      parked(false),
      parkAlt(0),
      parkAz(0),
      ////Target
      targetRA(-1),
      targetDec(-1),
      nextTargetRA(-1),
      nextTargetDec(-1),
      ////Tracking
      isTracking(true) {
  // Set inital pos
  phy.setAltAz(0, 0);
  // Some helper responses
  auto unimplemented = error(1024, "Property or Method Not Implemented");
  auto ascomFALSE = constant(false);
  auto ascomTRUE = constant(true);

  // Basic Info
  get(URL "name", constant("Pointer Bot"));
  get(URL "description", constant("ESP32 Alpaca Pointer"));
  get(URL "interfaceversion", constant(2));
  get(URL "driverinfo", constant("telescope"));
  get(URL "driverversion", constant("0.1"));

  // Connected
  prop(URL "connected",
       // Retrieves the connected state of the device
       VALUE(connected),
       // Sets the connected state of the device
       function("Connected", [this](String v) {
         return connected = v.equalsIgnoreCase("true");
       }));

  // Actions
  // Returns the list of action names supported by this driver.
  get(URL "supportedactions", unimplemented);
  // Invokes the named device-specific action.
  put(URL "action", unimplemented);

  // Commands
  // Transmits an arbitrary string to the device
  put(URL "commandblind", unimplemented);
  // Transmits an arbitrary string to the device and returns a boolean value
  put(URL "commandbool", unimplemented);
  // Transmits an arbitrary string to the device and returns a string value
  put(URL "commandstring", unimplemented);

  // Home
  // Indicates whether the mount can find the home position.
  get(URL "canfindhome", ascomFALSE);
  // Indicates whether the mount is at the home position.
  get(URL "athome", VALUE(phy.getAlt() == 0 && phy.getAz() == 0));
  // Moves the mount to the "home" position.
  put(URL "findhome", unimplemented);

  // Park
  // Indicates whether the telescope can be parked.
  get(URL "canpark", ascomTRUE);
  // Indicates whether the telescope park position can be set.
  get(URL "cansetpark", ascomTRUE);
  // Indicates whether the telescope can unpark
  get(URL "canunpark", ascomTRUE);
  // Indicates whether the telescope is at the park position.
  get(URL "atpark",
      VALUE(parked && phy.getAlt() == parkAlt && phy.getAz() == parkAz));
  // Park the mount
  put(URL "park", command([this]() {
        phy.setAltAz(0, 0);
        parked = true;
      }));
  // Sets the telescope's park position
  put(URL "setpark", command([this]() {
        parkAlt = phy.getAz();
        parkAz = phy.getAz();
      }));
  // Unparks the mount.
  put(URL "unpark", command([this]() { parked = false; }));

  // Optics
  // Returns the telescope's aperture.
  get(URL "aperturearea", unimplemented);
  // Returns the telescope's effective aperture.
  get(URL "aperturediameter", unimplemented);
  // Returns the telescope's focal length in meters.
  get(URL "focallength", unimplemented);
  // Indicates whether atmospheric refraction is applied to coordinates.
  get(URL "doesrefraction", unimplemented);
  // Determines whether atmospheric refraction is applied to coordinates.
  put(URL "doesrefraction", unimplemented);

  // Tracking
  // Returns the telescope's declination tracking rate.
  // Sets the telescope's declination tracking rate.
  prop(URL "declinationrate", constant(0), unimplemented);
  // Returns the telescope's right ascension tracking rate.
  // Sets the telescope's right ascension tracking rate.
  prop(URL "rightascensionrate", constant(0), unimplemented);

  // Indicates whether the Tracking property can be changed.
  get(URL "cansettracking", ascomTRUE);

  // Tracking
  prop(URL "tracking",
       // Indicates whether the telescope is tracking.
       VALUE(isTracking),
       // Enables or disables telescope tracking.
       function("Tracking", [this](String v) {
         return isTracking = v.equalsIgnoreCase("true");
       }));

  // Returns the current tracking rate.
  // Sets the mount's tracking rate.
  prop(URL "trackingrate", constant(0), unimplemented);

  // Returns a collection of supported DriveRates values.
  get(URL "trackingrates", producer([]() {
        StaticJsonDocument<JSON_ARRAY_SIZE(1)> doc;
        JsonArray array = doc.to<JsonArray>();
        array.add(0);
        return array;
      }));

  // Guiding
  // Indicates whether the telescope can be pulse guided.
  get(URL "canpulseguide", ascomFALSE);
  // Indicates whether the DeclinationRate property can be changed.
  get(URL "cansetguiderates", ascomFALSE);
  // Returns the current Declination rate offset for telescope guiding
  get(URL "guideratedeclination", unimplemented);
  // Sets the current Declination rate offset for telescope guiding.
  put(URL "guideratedeclination", unimplemented);
  // Returns the current RightAscension rate offset for telescope guiding
  get(URL "guideraterightascension", unimplemented);
  // Sets the current RightAscension rate offset for telescope guiding.
  put(URL "guideraterightascension", unimplemented);
  // Moves the scope in the given direction for the given time.
  put(URL "pulseguide", unimplemented);
  // Indicates whether the telescope is currently executing a PulseGuide command
  get(URL "ispulseguiding", unimplemented);

  // Sync
  // Syncs to the given local horizontal coordinates.
  put(URL "synctoaltaz", unimplemented);
  // Syncs to the given equatorial coordinates.
  put(URL "synctocoordinates", unimplemented);
  // Syncs to the TargetRightAscension and TargetDeclination coordinates.
  put(URL "synctotarget", unimplemented);

  // Site
  // Returns the observing site's elevation above mean sea level.
  // Sets the observing site's elevation above mean sea level.
  prop(URL "siteelevation", unimplemented, unimplemented);
  // Latitude
  prop(URL "sitelatitude",
       // Returns the observing site's latitude.
       VALUE(as.getLat()),
       // Sets the observing site's latitude.
       consumer("SiteLatitude", [this](String v) {
         double lat = v.toDouble();
         if (lat > 90 || lat < -90) {
           throw ASCOM_INVALID(Latitude);
         }
         as.setLat(lat);
       }));
  // Longitude
  prop(URL "sitelongitude",
       // Returns the observing site's longitude.
       VALUE(as.getLon()),
       // Sets the observing site's longitude.
       consumer("SiteLongitude", [this](String v) {
         double lon = v.toDouble();
         if (lon > 180 || lon < -180) {
           throw ASCOM_INVALID(Longitude);
         }
         as.setLon(lon);
       }));

  // Pier
  // Indicates whether the telescope SideOfPier can be set.
  get(URL "cansetpierside", ascomFALSE);
  // Returns the mount's pointing state.
  get(URL "sideofpier", unimplemented);
  // Sets the mount's pointing state.
  put(URL "sideofpier", unimplemented);
  // Predicts the pointing state after a German equatorial mount slews to given
  // coordinates.
  get(URL "destinationsideofpier", unimplemented);

  // Alignment Mode
  // Returns the current mount alignment mode
  get(URL "alignmentmode", constant(0));
  // Returns the current equatorial coordinate system used by this telescope.
  get(URL "equatorialsystem", constant(1));

  // Slewing

  // Indicates whether the telescope can slew synchronously.
  get(URL "canslew", ascomFALSE);
  // Indicates whether the telescope can slew asynchronously.
  get(URL "canslewasync", ascomTRUE);

  //////Slew - Equitorial
  // Synchronously slew to the given equatorial coordinates.
  put(URL "slewtocoordinates", unimplemented);
  // Asynchronously slew to the given equatorial coordinates.
  put(URL "slewtocoordinatesasync",
      consumer([this](AsyncWebServerRequest *req) {
        if (parked) throw ASCOM_INVALID_WHILE_PARKED(SlewToCoordinatesAsync);
        double ra = req->getParam("RightAscension", true)->value().toDouble();
        double dec = req->getParam("Declination", true)->value().toDouble();

        if (ra < 0 || ra > 24) {
          throw ASCOM_INVALID(Right Ascension);
        }
        if (dec < -90 || dec > 90) {
          throw ASCOM_INVALID(Declination);
        }

        targetRA = nextTargetRA = ra;
        targetDec = nextTargetDec = dec;

        double alt, az;
        as.convert(time(NULL), (targetRA / 24.f) * 360.0f, targetDec, &alt,
                   &az);
        phy.setAltAz(alt, az);
      }));

  //////Slew - RA /Dec

  // Target Declination
  prop(URL "targetdeclination",
       // Returns the current target declination.
       producer([this]() {
         if (nextTargetDec == -1) throw ASCOM_UNSET(Declination);
         return nextTargetDec;
       }),
       // Sets the target declination of a slew or sync.
       consumer("TargetDeclination", [this](String v) {
         double dec = v.toDouble();
         if (dec < -90 || dec > 90) {
           throw ASCOM_INVALID(Declination);
         }
         nextTargetDec = dec;
       }));

  // Returns the current target right ascension.
  prop(URL "targetrightascension", producer([this]() {
         if (nextTargetRA == -1) throw ASCOM_UNSET(Right Ascension);
         return nextTargetRA;
       }),
       // Sets the target right ascension of a slew or sync.
       consumer("TargetRightAscension", [this](String v) {
         double ra = v.toDouble();
         if (ra < 0 || ra > 24) {
           throw ASCOM_INVALID(Right Ascension);
         }
         nextTargetRA = ra;
       }));

  // Synchronously slew to the TargetRightAscension and TargetDeclination
  put(URL "slewtotarget", unimplemented);
  // Asynchronously slew to the TargetRightAscension and TargetDeclination
  put(URL "slewtotargetasync", command([this]() {
        if (parked) throw ASCOM_INVALID_WHILE_PARKED(SlewToCoordinatesAsync);
        targetRA = nextTargetRA;
        targetDec = nextTargetDec;

        double alt, az;
        as.convert(time(NULL), (targetRA / 24.f) * 360.0f, targetDec, &alt,
                   &az);
        phy.setAltAz(alt, az);
      }));

  //////Slew - Alt / Az
  // Indicates whether the telescope can slew synchronously to AltAz
  get(URL "canslewaltaz", ascomFALSE);
  // Indicates whether the telescope can slew asynchronously to AltAz
  get(URL "canslewaltazasync", ascomTRUE);
  // Returns the mount's altitude above the horizon.
  get(URL "altitude", VALUE(phy.getAlt()));
  // Returns the mount's azimuth.
  get(URL "azimuth", VALUE(phy.getAz()));
  // Synchronously slew to the given local horizontal coordinates.
  put(URL "slewtoaltaz", unimplemented);
  // Asynchronously slew to the given local horizontal coordinates.
  put(URL "slewtoaltazasync", consumer([this](AsyncWebServerRequest *request) {
        double alt = request->getParam("Altitude", true)->value().toDouble();
        double az = request->getParam("Azimuth", true)->value().toDouble();
        phy.setAltAz(alt, az);
      }));

  // Sync
  // Indicates whether the telescope can sync to equatorial coordinates.
  get(URL "cansync", ascomFALSE);
  // Indicates whether the telescope can sync to local horizontal coordinates.
  get(URL "cansyncaltaz", ascomFALSE);

  // Rates
  // Indicates whether the DeclinationRate property can be changed.
  get(URL "cansetdeclinationrate", ascomFALSE);
  // Indicates whether the RightAscensionRate property can be changed.
  get(URL "cansetrightascensionrate", ascomFALSE);

  // Motion
  // Indicates whether the telescope is currently slewing.
  get(URL "slewing", VALUE(phy.isMoving()));
  // Returns the post-slew settling time.
  // Sets the post-slew settling time.
  prop(URL "slewsettletime", unimplemented, unimplemented);
  // Immediatley stops a slew in progress.
  put(URL "abortslew", unimplemented);

  // Current Position
  // Returns the mount's declination.
  get(URL "declination", producer([this]() {
        double ra, dec;
        as.unconvert(time(NULL), phy.getAlt(), phy.getAz(), &ra, &dec);
        return dec;
      }));
  // Returns the mount's right ascension coordinate.
  get(URL "rightascension", producer([this]() {
        double ra, dec;
        as.unconvert(time(NULL), phy.getAlt(), phy.getAz(), &ra, &dec);
        return (ra / 360.0f) * 24.0f;
      }));

  // Time
  // Returns the local apparent sidereal time.
  get(URL "siderealtime",
      VALUE((as.localSiderealTime(time(NULL)) / 360.0f) * 24.0f));
  // Returns the UTC date/time of the telescope's internal clock.
  // Sets the UTC date/time of the telescope's internal clock.
  prop(URL "utcdate", producer(getCurrentTimeFormatted), unimplemented);

  // Move Axis
  // Indicates whether the telescope can move the requested axis.
  get(URL "canmoveaxis", ascomFALSE);
  // Returns the rates at which the telescope may be moved about the specified
  // axis.
  get(URL "axisrates", producer([]() {
        StaticJsonDocument<JSON_ARRAY_SIZE(1)> doc;
        JsonArray array = doc.to<JsonArray>();
        return array;
      }));
  // Moves a telescope axis at the given rate.
  put(URL "moveaxis", unimplemented);
}

std::string getCurrentTimeFormatted() {
  // yyyy-MM-ddTHH:mm:ss.fffffffZ
  time_t rawtime;
  struct tm *info;
  time(&rawtime);
  info = gmtime(&rawtime);

  std::ostringstream ss;
  ss << (1900 + info->tm_year);
  ss << "-";
  ss << std::setw(2) << std::setfill('0') << (1 + info->tm_mon);
  ss << "-";
  ss << std::setw(2) << std::setfill('0') << info->tm_mday;
  ss << "T";
  ss << std::setw(2) << std::setfill('0') << info->tm_hour;
  ss << ":";
  ss << std::setw(2) << std::setfill('0') << info->tm_min;
  ss << ":";
  ss << std::setw(2) << std::setfill('0') << info->tm_sec;
  ss << ".0000000Z";
  return ss.str();
}
