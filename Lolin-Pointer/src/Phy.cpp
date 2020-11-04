#include "Phy.h"
#include "Error.h"

#define AZ_STEPS 200
#define AZ_MICRO_STEPS 8
#define AZ_RING_TEETH 60
#define AZ_DRIVE_TEETH 15
#define AZ_STEPS_PER_REV \
  ((AZ_RING_TEETH * AZ_STEPS * AZ_MICRO_STEPS) / AZ_DRIVE_TEETH)

#define ALT_STEPS 200
#define ALT_MICRO_STEPS 8
#define ALT_GEAR_TEETH 35
#define ALT_DRIVE_TEETH 15
#define ALT_STEPS_PER_REV \
  ((ALT_GEAR_TEETH * ALT_STEPS * ALT_MICRO_STEPS) / ALT_DRIVE_TEETH)

#define ALT_MAX (ALT_STEPS_PER_REV / 2)
#define ALT_MIN (-ALT_MAX)

#define AZ_MAX (AZ_STEPS_PER_REV / 2)
#define AZ_MIN (-AZ_MAX)

#define AZ_STEP 17
#define AZ_DIR 16

#define ALT_STEP 23
#define ALT_DIR 18

#define STEP_DELAY 500

#define STEP(PIN, DIR)              \
  {                                 \
    digitalWrite(PIN##_DIR, DIR);   \
    digitalWrite(PIN##_STEP, HIGH); \
    digitalWrite(PIN##_STEP, LOW);  \
  }

#define ZERO(PIN) !digitalRead(PIN##_ZERO)

Phy::Phy() : alt_cur(0), az_cur(0), alt_target(0), az_target(0), azF(0), altF(0) {
  pinMode(AZ_STEP, OUTPUT);
  pinMode(AZ_DIR, OUTPUT);
  pinMode(ALT_STEP, OUTPUT);
  pinMode(ALT_DIR, OUTPUT);
  setAltAz(0,0); //initialize all bresenham variables
}

float Phy::getAlt() {
  float ret = (alt_cur - (az_cur * ALT_STEPS * ALT_MICRO_STEPS) / AZ_STEPS_PER_REV) * 360.0 / (float)ALT_STEPS_PER_REV;
  //TODO Need to un-adjust
  return ret;
}

float Phy::getAz() {
  return az_cur * 360.0 / (float)AZ_STEPS_PER_REV;
}

bool Phy::isMoving(){
  return moving;
}

void Phy::setAltAz(float altD, float azD) {
  Serial.print("setAltAz: ");
  Serial.print(altD);
  Serial.print(", Az: ");
  Serial.print(azD);
  Serial.println();

  if ( altD < 0 || altD > 90 )
    throw ASCOM_INVALID(Altitude);
  if ( azD < 0 || azD > 360 )
    throw ASCOM_INVALID(Azimuth);

  Serial.print("Setting Target Alt: ");
  Serial.print(altD);
  Serial.print(", Az: ");
  Serial.print(azD);
  Serial.println();


  az_target = AZ_STEPS_PER_REV * (azD / 360.0);
  int altExtra = (az_target * ALT_STEPS * ALT_MICRO_STEPS) / AZ_STEPS_PER_REV;
  alt_target = altExtra + ALT_STEPS_PER_REV * (altD / 360.0);
  

  moving = true;
}

void Phy::tick() {
  int alt_delta = alt_target - alt_cur;
  int az_delta = az_target - az_cur;

  float div = max(abs(alt_delta), abs(az_delta));
  float alt_step = alt_delta / div;
  float az_step = az_delta / div;

  moving = false;

	if (alt_cur != alt_target) {
    moving = true;
    altF = altF + alt_step;
    if (round(abs(alt_cur - altF)) > 1){
      STEP(ALT, alt_step<0);
      alt_cur += alt_step>0?1:-1;
    }
	}
	if (az_cur != az_target) {
    moving = true;
    azF = azF + az_step;
    if (round(abs(az_cur - azF)) > 1){
		  STEP(AZ, az_step<0);
      az_cur += az_step>0?1:-1;
    }
	}
  return;
}


void Phy::azCalCircle() {
  delay(3000);
  for (int i = 0; i < AZ_STEPS_PER_REV; i++) STEP(AZ, 0);
  delay(3000);
  for (int i = 0; i < AZ_STEPS_PER_REV; i++) STEP(AZ, 1);
}

void Phy::altCalCircle() {
  delay(3000);
  for (int i = 0; i < ALT_STEPS_PER_REV; i++) STEP(ALT, 0);
  delay(3000);
  for (int i = 0; i < ALT_STEPS_PER_REV; i++) STEP(ALT, 1);
}