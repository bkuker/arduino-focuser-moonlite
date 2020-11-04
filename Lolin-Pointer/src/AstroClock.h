/*
 * AstroClock.h
 *
 *  Created on: Jun 14, 2015
 *      Author: bkuker
 */

#ifndef ASTROCLOCK_H_
#define ASTROCLOCK_H_
#include "Arduino.h"

class AstroClock {
private:
	float lat;
	float lon;
	float convertJ2000(uint32_t timestamp);
	float getLSTd(float j2k);
public:
	AstroClock(float _lat, float _lon);
	virtual ~AstroClock();

	float getLat();
	void setLat(float _lat);
	float getLon();
	void setLon(float _lon);

	float localSiderealTime(uint32_t timestamp);

	void convert(uint32_t timestamp, float ra, float dec, float* alt,
			float* az);

	void unconvert(uint32_t timestamp, float alt, float az, float* ra,
			float* dec);
};

#endif /* ASTROCLOCK_H_ */
