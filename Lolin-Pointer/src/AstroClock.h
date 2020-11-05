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
	double lat;
	double lon;
	double convertJ2000(uint32_t timestamp);
	double getLSTd(double j2k);
public:
	AstroClock(double _lat, double _lon);
	virtual ~AstroClock();

	double getLat();
	void setLat(double _lat);
	double getLon();
	void setLon(double _lon);

	double localSiderealTime(uint32_t timestamp);

	void convert(uint32_t timestamp, double ra, double dec, double* alt,
			double* az);

	void unconvert(uint32_t timestamp, double alt, double az, double* ra,
			double* dec);
};

#endif /* ASTROCLOCK_H_ */
