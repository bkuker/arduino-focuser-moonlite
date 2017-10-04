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


#include <AccelStepper.h>


// maximum speed is 160pps which should be OK for most
// tin can steppers
#define MAXSPEED 100

/* Microstepping mode of driver (microsteps/full step) */
#define MICROSTEPS 1
/* The gear ratio of the stepper */
#define GEAR_RATIO 64
/* How many steps per full revolution of the motor itself (not including gearing) */
#define NATIVE_STEPS_PER_REV 48
/* How many pulses of the STEP pin for one revolution of gear shaft */
#define STEPS_PER_REV (NATIVE_STEPS_PER_REV * GEAR_RATIO * MICROSTEPS) 




#define MY_MS1_PIN 13
#define MY_MS2_PIN 15
#define MY_MS3_PIN 16
#define MY_SLP_PIN 19
#define MY_STEP_PIN 17
#define MY_DIR_PIN 18

#define TEMP_AVE_SEC 20

AccelStepper stepper(1, MY_STEP_PIN, MY_DIR_PIN);

#define MAXCOMMAND 8

char inChar;
char cmd[MAXCOMMAND];
char param[MAXCOMMAND];
char line[MAXCOMMAND];
long pos;
int isRunning = 0;
int speed = 32;
int eoc = 0;
int idx = 0;
long millisLastMove = 0;

void setup()
{  
  pinMode(MY_MS1_PIN, OUTPUT);
  digitalWrite(MY_MS1_PIN, LOW);
  pinMode(MY_MS2_PIN, OUTPUT);
  digitalWrite(MY_MS2_PIN, LOW);
  pinMode(MY_MS3_PIN, OUTPUT);
  digitalWrite(MY_MS3_PIN, LOW); 
  Serial.begin(9600);

  // we ignore the Moonlite speed setting because Accelstepper implements
  // ramping, making variable speeds un-necessary
  stepper.setSpeed(STEPS_PER_REV / 4);
  stepper.setMaxSpeed(STEPS_PER_REV / 4);
  stepper.setAcceleration(20);
  stepper.enableOutputs();
  stepper.setEnablePin(MY_SLP_PIN);
  memset(line, 0, MAXCOMMAND);
  millisLastMove = millis();
}



void loop(){

  // run the stepper if there's no pending command and if there are pending movements
  if (!Serial.available())
  {
    if (isRunning) {
      stepper.run();
      millisLastMove = millis();
    } 
    else {
      // reported on INDI forum that some steppers "stutter" if disableOutputs is done repeatedly
      // over a short interval; hence we only disable the outputs and release the motor some seconds
      // after movement has stopped
      if ((millis() - millisLastMove) > 15000) {
        stepper.disableOutputs();
      }
    }

    if (stepper.distanceToGo() == 0) {
      stepper.run();
      isRunning = 0;
    }
  } 
  else {

    // read the command until the terminating # character
    while (Serial.available() && !eoc) {
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
    }
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
      pos = stepper.currentPosition();
      char tempString[6];
      sprintf(tempString, "%04X", pos);
      Serial.print(tempString);
      Serial.print("#");
    }

    // get the new motor position (target)
    if (!strcasecmp(cmd, "GN")) {
      pos = stepper.targetPosition();
      char tempString[6];
      sprintf(tempString, "%04X", pos);
      Serial.print(tempString);
      Serial.print("#");
    }

    // get the current temperature
    // The temperature is sent in .5 *C units
    if (!strcasecmp(cmd, "GT")) {
      Serial.print("0020#");
    }

    // get the temperature coefficient, hard-coded
    if (!strcasecmp(cmd, "GC")) {
      Serial.print("02#");
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

      // we ignore the Moonlite speed setting because Accelstepper implements
      // ramping, making variable speeds un-necessary

      // stepper.setSpeed(speed * SPEEDMULT);
      // stepper.setMaxSpeed(speed * SPEEDMULT);
      stepper.setSpeed(MAXSPEED);
      stepper.setMaxSpeed(MAXSPEED);
    }

    /* Get half-stepping */
    if (!strcasecmp(cmd, "GH")) {
      Serial.print("00#");
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
      stepper.setCurrentPosition(pos);
    }

    // set new motor position
    if (!strcasecmp(cmd, "SN")) {
      pos = hexstr2long(param);
      stepper.moveTo(pos);
    }

    //Actually start the move
    if (!strcasecmp(cmd, "FG")) {
      isRunning = 1;
      stepper.enableOutputs();
      delay(1);
    }

    // stop a move
    if (!strcasecmp(cmd, "FQ")) {
      isRunning = 0;
      stepper.moveTo(stepper.currentPosition());
      stepper.run();
    }

  }
} // end loop

long hexstr2long(char *line) {
  long ret = 0;

  ret = strtol(line, NULL, 16);
  return (ret);
}


