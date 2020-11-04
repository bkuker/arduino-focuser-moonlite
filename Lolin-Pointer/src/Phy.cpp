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

Phy::Phy() : alt_cur(0), az_cur(0), alt_target(0), az_target(0) {
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
  if ( altD < 0 || altD > 90 )
    throw ASCOM_INVALID(Altitude);
  if ( azD < 0 || azD > 360 )
    throw ASCOM_INVALID(Azimuth);

  Serial.print("Setting Target Alt: ");
  Serial.print(altD);
  Serial.print(", Az: ");
  Serial.print(azD);
  Serial.println();

  /*if (azD > 180) {
    Serial.println("flip");
    azD = azD - 180;
    altD = 90 + 90 - altD;
  }*/
  az_target = AZ_STEPS_PER_REV * (azD / 360.0);
  int azDiff = az_target - az_cur;

  /*
  Serial.print("Az steps: ");
  Serial.print(AZ_STEPS_PER_REV);
  Serial.print(", Alt steps: ");
  Serial.print(ALT_STEPS_PER_REV);
  Serial.println();
  */

  int altExtra = (azDiff * ALT_STEPS * ALT_MICRO_STEPS) / AZ_STEPS_PER_REV;

  /*
  Serial.print("Az Diff: ");
  Serial.print(azDiff);
  Serial.print(", Alt Extra: ");
  Serial.print(altExtra);
  Serial.println();
  */

  alt_target = altExtra + ALT_STEPS_PER_REV * (altD / 360.0);

  /*
  Serial.print("Target Alt: ");
  Serial.print(alt_target);
  Serial.print(", Az: ");
  Serial.print(az_target);
  Serial.println();
  */

  bresSetup(az_target, alt_target);
  moving = true;
}

void Phy::tick() {
  // Execute one step of the line drawing algorithm
  if (k < tt) {
    k = k + 1;
    if (d > 0) {
      x = x + q;
      y = y + s;
      d = d + 2 * (a + b);
      STEP(AZ, dirx);
      STEP(ALT, diry);
    } else {
      if (ff) {
        x = x + q;
        STEP(AZ, dirx);
      } else {
        y = y + s;
        STEP(ALT, diry);
      }
      d = d + 2 * a;
    }
    /*
	Serial.print("x,y = ");
    Serial.print(x);
    Serial.print(" , ");
    Serial.print(y);
    Serial.println();
	*/

	az_cur = x;
	alt_cur = y;
  } else if (moving ) {
    moving = false;
	  az_cur = az_target;
	  alt_cur = alt_target;
    Serial.println("Arrived at target");
  }
}

void Phy::bresSetup(float xf, float yf) {
  // xf and yf rapresent the final point
  float DX = (float)xf - (float)az_cur;
  float DY = (float)yf - (float)alt_cur;
  x = az_cur;
  y = alt_cur;
  boolean ff = true;

  if (abs(DX) < abs(DY)) {
    float C = DX;
    DX = DY;
    DY = C;
    ff = false;
  }  // case when DX>DY!!
  a = abs(DY);
  b = -abs(DX);
  // calculation of d0
  d = (float)2 * a + b;
  // s  and q are the increments of x and y respectively
  q = 1;
  s = 1;
  dirx = 0;
  diry = 0;
  if (ff) {
    if (DX < 0) {
      q = -1;
      dirx = 1;
    }
    if (DY < 0) {
      s = -1;
      diry = 1;
    }
  } else {
    if (DX < 0) {
      q = -1;
      diry = 1;
    }
    if (DY < 0) {
      s = -1;
      dirx = 1;
    }
  }
  k = 0;
  tt = round(-b);
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