// Moonlite-compatible stepper controller
//
// Uses AccelStepper (http://www.airspayce.com/mikem/arduino/AccelStepper/)
//
// Requires a 10uf - 100uf capacitor between RESET and GND on the motor shield; this prevents the
// Arduino from resetting on connect (via DTR going low).  Without the capacitor, this sketch works
// with the stand-alone Moonlite control program (non-ASCOM) but the ASCOM driver does not detect it.
// Adding the capacitor allows the Arduino to respond quickly enough to the ASCOM driver probe
//
// orly.andico@gmail.com, 13 April 2014

#include "src/AccelStepper/AccelStepper.h"
#include "src/OneWire/OneWire.h" 
#include "src/DallasTemperature/DallasTemperature.h"

/* Microstepping Settings.
 * Run full 16x microstepping all the time on hardware for smoothing and reducing resonance.
 * 
 * Multiply/devide commanded reported position by microsteps, so every 1 step from software results
 * in 16 microsteps. Driver is reset on startup to ensure we always stop on full steps. This makes
 * the position stay more stable when enabling / disabling the driver whild idle to reduce power
 * usage and stepper heat.
 */
#define MICROSTEPS 16
#define MICROSTEP_MULTIPLIER (half_step?MICROSTEPS:MICROSTEPS*2)

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
#define DIR_PIN  2
#define STEP_PIN 3
#define ENABLE_PIN 4
#define RESET_PIN 5 //Optional

/* Optional feature pins */
#define ONE_WIRE_BUS 10
#define LED_PIN 9

AccelStepper stepper(1, STEP_PIN, DIR_PIN);

#ifdef ONE_WIRE_BUS
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
DeviceAddress thermometer;
#endif


//Input Handling
#define MAXCOMMAND 8
char inChar;
char cmd[MAXCOMMAND];
char param[MAXCOMMAND];
char line[MAXCOMMAND];
int eoc = 0;
int idx = 0;

//Internal State
int lastTemp = 0;
long millisLastTemp = 0;
long millisLastMove = 0;

//Moonlite State
long pos;
int isRunning = 0;
int speed = 2;
int half_step = 0;
int light = 255;

void setup()
{  
  Serial.begin(9600);
  
  stepper.setMaxSpeed(MAXSPEED);
  stepper.setAcceleration(ACCELERATION);
  stepper.disableOutputs();
  stepper.setEnablePin(ENABLE_PIN);
  stepper.setPinsInverted(false,false,true);
  memset(line, 0, MAXCOMMAND);
  millisLastMove = millis();

#ifdef LED_PIN
  analogWrite(LED_PIN, light);
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

#ifdef ONE_WIRE_BUS
  //Start up sensors
  sensors.begin();
  sensors.getAddress(thermometer, 0);
  sensors.requestTemperaturesByAddress(thermometer);
  lastTemp = sensors.getTempC(thermometer)*2;
  sensors.setWaitForConversion(false);
#endif
}

void motion(){
  //Motion Controll
  if (isRunning) {
    stepper.run();
    millisLastMove = millis();
  } 
  else {
    // reported on INDI forum that some steppers "stutter" if disableOutputs is done repeatedly
    // over a short interval; hence we only disable the outputs and release the motor some seconds
    // after movement has stopped
    if ((millis() - millisLastMove) > SETTLE_MS) {
       stepper.disableOutputs();
    }
  }
  if (stepper.distanceToGo() == 0) {
    stepper.run();
    isRunning = 0;
  }
}

void loop(){
  motion();

  //Read One Character from serial
  if (Serial.available()) {
    inChar = Serial.read();
    if (inChar != '#' && inChar != ':') {
      line[idx++] = inChar;
      if (idx >= MAXCOMMAND) {
        idx = MAXCOMMAND - 1;
      }
    } 
    else {
      if (inChar == '#') {
        eoc = 1;
      }
    }
    motion();
  } // end if (!Serial.available())

  // process the command we got
  if (eoc) {
    memset(cmd, 0, MAXCOMMAND);
    memset(param, 0, MAXCOMMAND);

    int len = strlen(line);
    if (len >= 2) {
      strncpy(cmd, line, 2);
    }

    if (len > 2) {
      strncpy(param, line + 2, len - 2);
    }

    memset(line, 0, MAXCOMMAND);
    eoc = 0;
    idx = 0;

    // the stand-alone program sends :C# :GB# on startup
    // :C# is a temperature conversion, doesn't require any response

    // LED backlight value, always return "00"
    if (!strcasecmp(cmd, "GB")) {
      Serial.print("00#");
    }

    // home the motor, hard-coded, ignore parameters since we only have one motor
    if (!strcasecmp(cmd, "PH")) { 
      stepper.setCurrentPosition(8000);
      stepper.moveTo(0);
      isRunning = 1;
    }

    // firmware value, always return "10"
    if (!strcasecmp(cmd, "GV")) {
      Serial.print("10#");
    }

    // get the current motor position
    if (!strcasecmp(cmd, "GP")) {
      pos = stepper.currentPosition() / MICROSTEP_MULTIPLIER;
      char tempString[6];
      sprintf(tempString, "%04X", pos);
      Serial.print(tempString);
      Serial.print("#");
    }

    // get the new motor position (target)
    if (!strcasecmp(cmd, "GN")) {
      pos = stepper.targetPosition() / MICROSTEP_MULTIPLIER;
      char tempString[6];
      sprintf(tempString, "%04X", pos);
      Serial.print(tempString);
      Serial.print("#");
    }

    // get the current temperature
    // The temperature is sent in .5 *C units
    if (!strcasecmp(cmd, "GT")) {
#ifdef ONE_WIRE_BUS
    if ((millis() - millisLastTemp) > 1000 && !isRunning) {
      lastTemp = sensors.getTempC(thermometer)*2;
      motion();
      sensors.requestTemperaturesByAddress(thermometer);
      millisLastTemp = millis();
    }
#endif
      char tempString[6];
      sprintf(tempString, "%04X", lastTemp);
      Serial.print(tempString);
      Serial.print("#");
    }

    // get the temperature coefficient, hard-coded
    if (!strcasecmp(cmd, "GC")) {
      Serial.print("02#");
    }

    // get the current light
    if (!strcasecmp(cmd, "GB")) {
      char tempString[6];
      sprintf(tempString, "%02X", light);
      Serial.print(tempString);
      Serial.print("#");
    }

    //set the light
    if (!strcasecmp(cmd, "SB")) {
      light = hexstr2long(param);
#ifdef LED_PIN
      analogWrite(LED_PIN, light);
#endif
    }
    
    // get the current motor speed, only values of 02, 04, 08, 10, 20
    if (!strcasecmp(cmd, "GD")) {
      char tempString[6];
      sprintf(tempString, "%02X", speed);
      Serial.print(tempString);
      Serial.print("#");
    }

    // set speed, only acceptable values are 02, 04, 08, 10, 20
    if (!strcasecmp(cmd, "SD")) {
      speed = hexstr2long(param);
      //Setting the speed too close to the end causes accelstepper to
      //overshoot when moving positive, and freak out when moving negative
      if ( abs(stepper.distanceToGo()) > 20 ){
        stepper.setMaxSpeed(MAXSPEED * 2L / speed);
      }
    }

    /* Get half-stepping */
    if (!strcasecmp(cmd, "GH")) {
      if (half_step) {
        Serial.print("FF#");
      } else {
        Serial.print("00#");
      }
    }

    // motor is moving - 01 if moving, 00 otherwise
    if (!strcasecmp(cmd, "GI")) {
      if (abs(stepper.distanceToGo()) > 0) {
        Serial.print("01#");
      } 
      else {
        Serial.print("00#");
      }
    }

    // set current motor position
    if (!strcasecmp(cmd, "SP")) {
      pos = hexstr2long(param);
      stepper.setCurrentPosition(pos * MICROSTEP_MULTIPLIER);
    }

    // set new motor position
    if (!strcasecmp(cmd, "SN")) {
      pos = hexstr2long(param);
      stepper.moveTo(pos * MICROSTEP_MULTIPLIER);
    }

    /* Set half-step mode */
    if (!strcasecmp(cmd, "SH")) {
        half_step = 1;
    }

    /* Set full-step mode */
    if (!strcasecmp(cmd, "SF")) {
        half_step = 0;
    }

    //Actually start the move
    if (!strcasecmp(cmd, "FG")) {
      isRunning = 1;
      stepper.setMaxSpeed(MAXSPEED * 2L / speed);
      stepper.enableOutputs();
      delay(1);
    }

    // stop a move
    if (!strcasecmp(cmd, "FQ")) {
      isRunning = 0;
      stepper.moveTo(stepper.currentPosition());
      stepper.run();
      stepper.disableOutputs();
    }

  }
} // end loop

long hexstr2long(char *line) {
  long ret = 0;

  ret = strtol(line, NULL, 16);
  return (ret);
}
