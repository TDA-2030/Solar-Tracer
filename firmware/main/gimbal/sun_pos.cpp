// This file is available in electronic form at http://www.psa.es/sdg/sunpos.htm

#include "sun_pos.h"
#include <math.h>

#ifdef __linux__
#include <cstdio>
#endif

void sunpos(cTime udtTime,cLocation udtLocation, cSunCoordinates *udtSunCoordinates)
{
	// Main variables
	double dElapsedJulianDays;
	double dDecimalHours;
	double dEclipticLongitude;
	double dEclipticObliquity;
	double dRightAscension;
	double dDeclination;

	// Auxiliary variables
	double dY;
	double dX;

	// Calculate difference in days between the current Julian Day 
	// and JD 2451545.0, which is noon 1 January 2000 Universal Time
	{
		double dJulianDate;
		long int liAux1;
		long int liAux2;
		// Calculate time of the day in UT decimal hours
		dDecimalHours = udtTime.dHours + (udtTime.dMinutes 
			+ udtTime.dSeconds / 60.0 ) / 60.0;
		// Calculate current Julian Day
		liAux1 =(udtTime.iMonth-14)/12;
		liAux2=(1461*(udtTime.iYear + 4800 + liAux1))/4 + (367*(udtTime.iMonth 
			- 2-12*liAux1))/12- (3*((udtTime.iYear + 4900 
		+ liAux1)/100))/4+udtTime.iDay-32075;
		dJulianDate=(double)(liAux2)-0.5+dDecimalHours/24.0;
		// Calculate difference between current Julian Day and JD 2451545.0 
		dElapsedJulianDays = dJulianDate-2451545.0;
		// printf("Julian Days: %f\n", dJulianDate);

	}

	// Calculate ecliptic coordinates (ecliptic longitude and obliquity of the 
	// ecliptic in radians but without limiting the angle to be less than 2*Pi 
	// (i.e., the result may be greater than 2*Pi)
	{
		double dMeanLongitude;
		double dMeanAnomaly;
		double dOmega;
		dOmega=2.1429-0.0010394594*dElapsedJulianDays;
		dMeanLongitude = 4.8950630+ 0.017202791698*dElapsedJulianDays; // Radians
		dMeanAnomaly = 6.2400600+ 0.0172019699*dElapsedJulianDays;
		dEclipticLongitude = dMeanLongitude + 0.03341607*sin( dMeanAnomaly ) 
			+ 0.00034894*sin( 2*dMeanAnomaly )-0.0001134
			-0.0000203*sin(dOmega);
		dEclipticObliquity = 0.4090928 - 6.2140e-9*dElapsedJulianDays
			+0.0000396*cos(dOmega);
	}

	// Calculate celestial coordinates ( right ascension and declination ) in radians 
	// but without limiting the angle to be less than 2*Pi (i.e., the result may be 
	// greater than 2*Pi)
	{
		double dSin_EclipticLongitude;
		dSin_EclipticLongitude= sin( dEclipticLongitude );
		dY = cos( dEclipticObliquity ) * dSin_EclipticLongitude;
		dX = cos( dEclipticLongitude );
		dRightAscension = atan2( dY,dX );
		if( dRightAscension < 0.0 ) dRightAscension = dRightAscension + twopi;
		dDeclination = asin( sin( dEclipticObliquity )*dSin_EclipticLongitude );
	}

	// Calculate local coordinates ( azimuth and zenith angle ) in degrees
	{
		double dGreenwichMeanSiderealTime;
		double dLocalMeanSiderealTime;
		double dLatitudeInRadians;
		double dHourAngle;
		double dCos_Latitude;
		double dSin_Latitude;
		double dCos_HourAngle;
		double dParallax;
		dGreenwichMeanSiderealTime = 6.6974243242 + 
			0.0657098283*dElapsedJulianDays 
			+ dDecimalHours;
		dLocalMeanSiderealTime = (dGreenwichMeanSiderealTime*15 
			+ udtLocation.dLongitude)*rad;
		dHourAngle = dLocalMeanSiderealTime - dRightAscension;
		dLatitudeInRadians = udtLocation.dLatitude*rad;
		dCos_Latitude = cos( dLatitudeInRadians );
		dSin_Latitude = sin( dLatitudeInRadians );
		dCos_HourAngle= cos( dHourAngle );
		udtSunCoordinates->dZenithAngle = (acos( dCos_Latitude*dCos_HourAngle
			*cos(dDeclination) + sin( dDeclination )*dSin_Latitude));
		dY = -sin( dHourAngle );
		dX = tan( dDeclination )*dCos_Latitude - dSin_Latitude*dCos_HourAngle;
		udtSunCoordinates->dAzimuth = atan2( dY, dX );
		if ( udtSunCoordinates->dAzimuth < 0.0 ) 
			udtSunCoordinates->dAzimuth = udtSunCoordinates->dAzimuth + twopi;
		udtSunCoordinates->dAzimuth = udtSunCoordinates->dAzimuth/rad;
		// Parallax Correction
		dParallax=(dEarthMeanRadius/dAstronomicalUnit)
			*sin(udtSunCoordinates->dZenithAngle);
		udtSunCoordinates->dZenithAngle=(udtSunCoordinates->dZenithAngle 
			+ dParallax)/rad;
		udtSunCoordinates->dElevation = 90-udtSunCoordinates->dZenithAngle;
	}
}

// run test on linux
#ifdef __linux__

#include <time.h>

void search_azimuth(float *max_azimuth, float *min_azimuth)
{
    *max_azimuth = 0;
    *min_azimuth = 360;
    // search maximum and minimum azimuth in a day
    time_t now;
    struct tm local_time;
    time(&now);
    localtime_r(&now, &local_time);

    local_time.tm_hour = 6;
    local_time.tm_min = 0;
    local_time.tm_sec = 0;
	time_t start_time = mktime(&local_time);

    // 假设的 GPS 数据
    struct cLocation location;
	location.dLongitude = 112.933333;
    location.dLatitude = 28.183333;

    // 模拟从北京时间6点到18点每隔30分钟计算一次
    for (int i = 0; i < 25; i++) { // 从6点到18点有25个
        // 转换为UTC时间
        struct tm utcTime;
		gmtime_r(&start_time, &utcTime);

        // 转换为cTime结构体
        cTime utcTimeStruct = {
            .iYear = utcTime.tm_year + 1900,
            .iMonth = utcTime.tm_mon + 1,
            .iDay = utcTime.tm_mday,
            .dHours = utcTime.tm_hour,
            .dMinutes = utcTime.tm_min,
            .dSeconds = utcTime.tm_sec
        };

        cSunCoordinates sunCoordinates;
        sunpos(utcTimeStruct, location, &sunCoordinates);

        // 更新Azimuth的最大值和最小值
        if (sunCoordinates.dAzimuth > *max_azimuth) {
            *max_azimuth = sunCoordinates.dAzimuth;
        }
        if (sunCoordinates.dAzimuth < *min_azimuth) {
            *min_azimuth = sunCoordinates.dAzimuth;
        }

        // 打印结果
        printf("UTC Time: %d-%02d-%02d %02d:%02d:%02d  Elevation: %.2f°, Azimuth: %.2f°, Zenith: %.2f°\n",
               utcTimeStruct.iYear, utcTimeStruct.iMonth, utcTimeStruct.iDay, (int)utcTimeStruct.dHours, (int)utcTimeStruct.dMinutes, (int)utcTimeStruct.dSeconds,
               sunCoordinates.dElevation, sunCoordinates.dAzimuth, sunCoordinates.dZenithAngle);

        // 更新时间为下一个30分钟
        start_time += 30 * 60;
    }

    // 打印Azimuth的最大值和最小值
    printf("Maximum Azimuth: %.2f°\n", *max_azimuth);
    printf("Minimum Azimuth: %.2f°\n", *min_azimuth);
}


int main(int argc, char **argv)
{
	if (argc != 6)
	{
		printf("Usage: sun_pos <year> <month> <day> <hour> <minute>\n");
		return 1;
	}

	struct cTime time;
	time.iYear = atoi(argv[1]);
	time.iMonth = atoi(argv[2]);
	time.iDay = atoi(argv[3]);
	time.dHours = atoi(argv[4]);
	time.dMinutes = atoi(argv[5]);
	time.dSeconds = 0;

	struct cLocation location;
	location.dLongitude = 112.933333;
    location.dLatitude = 28.183333;

	struct cSunCoordinates sunCoordinates;
	sunpos(time, location, &sunCoordinates);
	printf("%f\t%f\t%f\n", sunCoordinates.dAzimuth, sunCoordinates.dZenithAngle, sunCoordinates.dElevation);
	
	float max_azimuth, min_azimuth;
	search_azimuth(&max_azimuth, &min_azimuth);
	return 0;
}

#endif