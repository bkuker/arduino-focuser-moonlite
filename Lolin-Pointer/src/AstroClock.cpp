/*
 * AstroClock.cpp
 *
 *  Created on: Jun 14, 2015
 *      Author: bkuker
 */

#include "AstroClock.h"
#include <math.h>
#include <stdint.h>

#define J2K 946728000

#define RAD(D) (D / 57.2957795)
#define DEG(R) (R * 57.2957795)

#define SIN(D) sin(RAD(D))
#define COS(D) cos(RAD(D))
#define ASIN(X) DEG(asin(X))
#define ACOS(X) DEG(acos(X))

AstroClock::AstroClock(double _lat, double _lon) :
		lat(_lat), lon(_lon) {
}

double AstroClock::getLat(){
	return lat;
}

void AstroClock::setLat(double _lat){
	lat = _lat;
}

double AstroClock::getLon(){
	return lon;
}

void AstroClock::setLon(double _lon){
	lon = _lon;
}


AstroClock::~AstroClock() {
	// TODO Auto-generated destructor stub
}

double AstroClock::convertJ2000(uint32_t timestamp) {
	uint32_t seconds = timestamp - J2K;
	return seconds / (60.0 * 60.0 * 24.0);
}

double AstroClock::getLSTd(double j2k) {
	double ut = 12 + 24 * (j2k - floor(j2k));
	double lstd = 100.46 + 0.985647 * j2k + lon + 15 * ut;
	while (lstd - 360.0 > 0)
		lstd = lstd - 360.0;
	return lstd;
}

void AstroClock::convert(uint32_t timestamp, double ra, double dec, double* alt,
		double* az) {
	double lstd = getLSTd(convertJ2000(timestamp));
	double ha = lstd - ra;
	while (ha < 0)
		ha += 360.0;

	double sinAlt = SIN(dec) * SIN(lat) + COS(dec) * COS(lat) * COS(ha);
	*alt = ASIN(sinAlt);

	double cosA = //
			(SIN(dec) - SIN(*alt) * SIN(lat)) //
			/ (COS(*alt) * COS(lat));

	double a = ACOS(cosA);

	if (SIN(ha) < 0)
		*az = a;
	else
		*az = 360.0 - a;
}

double AstroClock::localSiderealTime(uint32_t timestamp){
	return getLSTd(convertJ2000(timestamp));
}

void AstroClock::unconvert(uint32_t timestamp, double alt, double az,
		double* raOut, double* decOut) {

	double d2r = PI / 180.0f;

	double alt_r = alt * d2r;
	double az_r = az * d2r;
	double lat_r = lat * d2r;

	//******************************************************************************
	//find local HOUR ANGLE (in degrees, from 0. to 360.)
	double ha = atan2(-sin(az_r) * cos(alt_r),
			-cos(az_r) * sin(lat_r) * cos(alt_r) + sin(alt_r) * cos(lat_r));
	ha = ha / d2r;

	// w = where(ha LT 0.)
	//if w[0] ne -1 then ha[w] = ha[w] + 360.
	//ha = ha mod 360.
	if (ha < 0) {
		ha += 360;
	}
	while (ha >= 360)
		ha = ha - 360;

	//Find declination (positive if north of Celestial Equator, negative if south)
	double sindec = sin(lat_r) * sin(alt_r)
			+ cos(lat_r) * cos(alt_r) * cos(az_r);
	double dec = asin(sindec) / d2r;	 // convert dec to degrees
	while ( dec < 0 )
		dec += 360;

	double lstd = getLSTd(convertJ2000(timestamp));

	*decOut = dec;
	*raOut = lstd - ha;

	while ( *raOut < 0 )
		*raOut += 360;

}
