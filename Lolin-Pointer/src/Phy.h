#ifndef PHY_H_
#define PHY_H_
#include "Arduino.h"

class Phy {
private:
	int alt_cur;
	int az_cur;
	int alt_target;
	int az_target;

	bool moving;

	void bresSetup(float xf, float yf);

	float azF, altF;
public:
	Phy();
	void setAltAz(float altD, float azD);
	float getAlt();
	float getAz();
	void tick();
	bool isMoving();

	void azCalCircle();
	void altCalCircle();
};

#endif
