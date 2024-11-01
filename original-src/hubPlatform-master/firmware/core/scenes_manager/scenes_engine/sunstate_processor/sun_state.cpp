/*
 * Provides the ability to calculate the local time for sunrise,
 * sunset, and moonrise at any point in time at any location in the world
 *
 * Original work used with permission maintaining license
 * Copyright (GPL) 2004 Mike Chirico mchirico@comcast.net
 * Modifications copyright
 * Copyright (GPL) 2015 Peter Buelow
 *
 * This file is part of the Sunset library
 *
 * Sunset is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Sunset is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "sun_state.h"
#include <utility>

SunState::SunState() {
    setPosition( 0, 0, 0 );
}

SunState::SunState( double lat, double lon, int timeZone ) {
    setPosition( lat, lon, timeZone );
}

SunState::SunState( double lat, double lon, const ClockGetter& clockGetter )
: tzOffset( clockGetter ) {
    if( clockGetter == nullptr )
        throw std::invalid_argument( "SunState: clock getter should not be null" );
    latitude = lat;
    longitude = lon;
}

SunState::~SunState() = default;

void SunState::setPosition( double lat, double lon, int timeZone ) {
    TimeZoneOffset tz(
        abs( timeZone ), 0, timeZone < 0 ? TimeZoneOffset::Direction::AFTER : TimeZoneOffset::Direction::BEFORE );
    setPosition( lat, lon, tz );
}

void SunState::setPosition( double lat, double lon, SunState::TimeZoneOffset timeZone ) {
    latitude = lat;
    longitude = lon;
    tzOffset = std::move( timeZone );
}

double SunState::degToRad( double angleDeg ) {
    return ( M_PI * angleDeg / 180.0 );
}

double SunState::radToDeg( double angleRad ) {
    return ( 180.0 * angleRad / M_PI );
}

double SunState::calcMeanObliquityOfEcliptic( double t ) {
    double seconds = 21.448 - t * ( 46.8150 + t * ( 0.00059 - t * ( 0.001813 ) ) );
    double e0 = 23.0 + ( 26.0 + ( seconds / 60.0 ) ) / 60.0;

    return e0;  // in degrees
}

double SunState::calcGeomMeanLongSun( double t ) {
    double L = 280.46646 + t * ( 36000.76983 + 0.0003032 * t );

    while( (int) L > 360 ) {
        L -= 360.0;
    }

    while( L < 0 ) {
        L += 360.0;
    }

    return L;  // in degrees
}

double SunState::calcObliquityCorrection( double t ) {
    double e0 = calcMeanObliquityOfEcliptic( t );
    double omega = 125.04 - 1934.136 * t;
    double e = e0 + 0.00256 * cos( degToRad( omega ) );

    return e;  // in degrees
}

double SunState::calcEccentricityEarthOrbit( double t ) {
    double e = 0.016708634 - t * ( 0.000042037 + 0.0000001267 * t );
    return e;  // unitless
}

double SunState::calcGeomMeanAnomalySun( double t ) {
    double M = 357.52911 + t * ( 35999.05029 - 0.0001537 * t );
    return M;  // in degrees
}

double SunState::calcEquationOfTime( double t ) {
    double epsilon = calcObliquityCorrection( t );
    double l0 = calcGeomMeanLongSun( t );
    double e = calcEccentricityEarthOrbit( t );
    double m = calcGeomMeanAnomalySun( t );
    double y = tan( degToRad( epsilon ) / 2.0 );

    y *= y;

    double sin2l0 = sin( 2.0 * degToRad( l0 ) );
    double sinm = sin( degToRad( m ) );
    double cos2l0 = cos( 2.0 * degToRad( l0 ) );
    double sin4l0 = sin( 4.0 * degToRad( l0 ) );
    double sin2m = sin( 2.0 * degToRad( m ) );
    double Etime =
        y * sin2l0 - 2.0 * e * sinm + 4.0 * e * y * sinm * cos2l0 - 0.5 * y * y * sin4l0 - 1.25 * e * e * sin2m;
    return radToDeg( Etime ) * 4.0;  // in minutes of time
}

double SunState::calcTimeJulianCent( double jd ) {
    double T = ( jd - 2451545.0 ) / 36525.0;
    return T;
}

double SunState::calcSunTrueLong( double t ) {
    double l0 = calcGeomMeanLongSun( t );
    double c = calcSunEqOfCenter( t );

    double O = l0 + c;
    return O;  // in degrees
}

double SunState::calcSunApparentLong( double t ) {
    double o = calcSunTrueLong( t );

    double omega = 125.04 - 1934.136 * t;
    double lambda = o - 0.00569 - 0.00478 * sin( degToRad( omega ) );
    return lambda;  // in degrees
}

double SunState::calcSunDeclination( double t ) {
    double e = calcObliquityCorrection( t );
    double lambda = calcSunApparentLong( t );

    double sint = sin( degToRad( e ) ) * sin( degToRad( lambda ) );
    double theta = radToDeg( asin( sint ) );
    return theta;  // in degrees
}

double SunState::calcHourAngleSunrise( double lat, double solarDec ) {
    double latRad = degToRad( lat );
    double sdRad = degToRad( solarDec );
    double HA = ( acos( cos( degToRad( 90.833 ) ) / ( cos( latRad ) * cos( sdRad ) ) - tan( latRad ) * tan( sdRad ) ) );

    return HA;  // in radians
}

double SunState::calcHourAngleSunset( double lat, double solarDec ) {
    double latRad = degToRad( lat );
    double sdRad = degToRad( solarDec );
    double HA = ( acos( cos( degToRad( 90.833 ) ) / ( cos( latRad ) * cos( sdRad ) ) - tan( latRad ) * tan( sdRad ) ) );

    return -HA;  // in radians
}

double SunState::calcJD( int y, int m, int d ) {
    if( m <= 2 ) {
        y -= 1;
        m += 12;
    }
    int A = floor( y / 100 );
    int B = 2 - A + floor( A / 4 );

    double JD = floor( 365.25 * ( y + 4716 ) ) + floor( 30.6001 * ( m + 1 ) ) + d + B - 1524.5;
    return JD;
}

double SunState::calcJDFromJulianCent( double t ) {
    double JD = t * 36525.0 + 2451545.0;
    return JD;
}

double SunState::calcSunEqOfCenter( double t ) {
    double m = calcGeomMeanAnomalySun( t );
    double mrad = degToRad( m );
    double sinm = sin( mrad );
    double sin2m = sin( mrad + mrad );
    double sin3m = sin( mrad + mrad + mrad );
    double C =
        sinm * ( 1.914602 - t * ( 0.004817 + 0.000014 * t ) ) + sin2m * ( 0.019993 - 0.000101 * t ) + sin3m * 0.000289;

    return C;  // in degrees
}

SunState::Time SunState::secondsToTime( double seconds ) {
    Time time{ NaN, NaN, NaN };
    if( isnan( seconds ) ) {
        return time;
    }
    time.hours = seconds / 3600;
    int hoursInSeconds = time.hours * 3600;
    time.minutes = ( seconds - hoursInSeconds ) / 60;
    time.seconds = seconds - hoursInSeconds - time.minutes * 60;
    return time;
}

/** Calculates sunrise offset in minutes from UTC midnight.
 * @returns number of minutes from midnight, UTC
 * */
double SunState::calcSunriseUTC() const {
    double t = calcTimeJulianCent( julianDate );
    // *** First pass to approximate sunrise
    double eqTime = calcEquationOfTime( t );
    double solarDec = calcSunDeclination( t );
    double hourAngle = calcHourAngleSunrise( latitude, solarDec );
    double delta = longitude + radToDeg( hourAngle );
    double timeDiff = 4 * delta;               // in minutes of time
    double timeUTC = 720 - timeDiff - eqTime;  // in minutes
    double newt = calcTimeJulianCent( calcJDFromJulianCent( t ) + timeUTC / 1440.0 );

    eqTime = calcEquationOfTime( newt );
    solarDec = calcSunDeclination( newt );

    hourAngle = calcHourAngleSunrise( latitude, solarDec );
    delta = longitude + radToDeg( hourAngle );
    timeDiff = 4 * delta;
    timeUTC = 720 - timeDiff - eqTime;  // in minutes

    return timeUTC;
}

/** Calculates sunrise offset in minutes from midnight.
 * @returns number of minutes from midnight, local time
 */
double SunState::getSunriseMinutes() const {
    double timeUTC = calcSunriseUTC();

    double localTime = timeUTC + tzOffset.getTimeZoneOffsetMinutes().count();

    return localTime;  // return time in minutes from midnight
}

double SunState::calcSunsetUTC() const {
    double t = calcTimeJulianCent( julianDate );
    // *** First pass to approximate sunset
    double eqTime = calcEquationOfTime( t );
    double solarDec = calcSunDeclination( t );
    double hourAngle = calcHourAngleSunset( latitude, solarDec );
    double delta = longitude + radToDeg( hourAngle );
    double timeDiff = 4 * delta;               // in minutes of time
    double timeUTC = 720 - timeDiff - eqTime;  // in minutes
    double newt = calcTimeJulianCent( calcJDFromJulianCent( t ) + timeUTC / 1440.0 );

    eqTime = calcEquationOfTime( newt );
    solarDec = calcSunDeclination( newt );

    hourAngle = calcHourAngleSunset( latitude, solarDec );
    delta = longitude + radToDeg( hourAngle );
    timeDiff = 4 * delta;
    timeUTC = 720 - timeDiff - eqTime;  // in minutes

    return timeUTC;  // return time in minutes from midnight
}

double SunState::getSunsetMinutes() const {
    double timeUTC = calcSunsetUTC();  // in minutes

    double localTime = timeUTC + tzOffset.getTimeZoneOffsetMinutes().count();

    return localTime;  // return time in minutes from midnight
}

double SunState::getSunriseSeconds() const {
    return getSunriseMinutes() * 60;
}

double SunState::getSunsetSeconds() const {
    return getSunsetMinutes() * 60;
}

double SunState::setCurrentDate( int year, int month, int day ) {
    m_year = year;
    m_month = month;
    m_day = day;
    julianDate = calcJD( year, month, day );
    return julianDate;
}

int SunState::moonPhase( int fromepoch ) {
    int moonepoch = 614100;

    int phase = ( fromepoch - moonepoch ) % 2551443;
    int res = floor( phase / ( 24 * 3600 ) ) + 1;
    if( res == 30 )
        res = 0;

    return res;
}

SunState::Time SunState::getSunriseTime() {
    return secondsToTime( getSunriseSeconds() );
}

SunState::Time SunState::getSunsetTime() {
    return secondsToTime( getSunsetSeconds() );
}

/** Compare two times. If seconds is NaN in one of the arguments, compare with respect to hours and minutes only. */
bool SunState::Time::operator==( const SunState::Time& right ) const {
    return hours == right.hours && minutes == right.minutes &&
           ( seconds == NaN || right.seconds == NaN ? true : seconds == right.seconds );
}

int SunState::Time::toSeconds() const {
    return ( hours * 60 + minutes ) * 60 + seconds;
}

SunState::TimeZoneOffset::TimeZoneOffset( uint8_t hours, uint8_t minutes, Direction direction )
: m_hours( hours )
, m_minutes( minutes )
, m_direction( direction ) {
    if( hours > 12 )
        throw std::invalid_argument( "Invalid hour offset passed: " + std::to_string( hours ) + ", should be <= 12." );
    if( minutes >= 60 )
        throw std::invalid_argument( "Invalid minutes offset passed: " + std::to_string( minutes ) +
                                     ", should be < 60." );
}

SunState::TimeZoneOffset::TimeZoneOffset( SunState::ClockGetter clockGetter )
: m_clockGetter( std::move( clockGetter ) ) {}

std::chrono::seconds SunState::TimeZoneOffset::getTimeZoneOffsetSeconds() const {
    if( m_clockGetter != nullptr ) {
        auto tzOffsetSeconds = m_clockGetter().timeZoneOffset();
        return std::chrono::duration_cast< std::chrono::seconds >( tzOffsetSeconds );
    }
    return std::chrono::seconds( ( m_hours * 60 + m_minutes ) * 60 * static_cast< int >( m_direction ) );
}

std::chrono::minutes SunState::TimeZoneOffset::getTimeZoneOffsetMinutes() const {
    if( m_clockGetter != nullptr ) {
        auto tzOffsetSeconds = m_clockGetter().timeZoneOffset();
        return std::chrono::duration_cast< std::chrono::minutes >( tzOffsetSeconds );
    }
    return std::chrono::minutes( ( m_hours * 60 + m_minutes ) * static_cast< int >( m_direction ) );
}

SunState::TimeZoneOffset::Direction SunState::TimeZoneOffset::getDirection() const {
    return m_direction;
}

int SunState::TimeZoneOffset::getHours() const {
    return m_hours;
}

int SunState::TimeZoneOffset::getMinutes() const {
    return m_minutes;
}
