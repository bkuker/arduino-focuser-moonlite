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

	void bresSetup(double xf, double yf);

	double azF, altF;
public:
	Phy();
	void setAltAz(double altD, double azD);
	double getAlt();
	double getAz();
	void tick();
	bool isMoving();

	void azCalCircle();
	void altCalCircle();
};

#endif
