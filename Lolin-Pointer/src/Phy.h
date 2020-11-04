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

	//bresenham line drawing state
	long k, tt;
	float x, y, d, a, b;
	int dirx, diry, q, s;
	bool ff;
public:
	Phy();
	void setAltAz(float altD, float azD);
	float getAlt();
	float getAz();
	void tick();

	void azCalCircle();
	void altCalCircle();
};

#endif
