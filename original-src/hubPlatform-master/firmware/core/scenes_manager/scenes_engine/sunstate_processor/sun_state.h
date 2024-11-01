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

#pragma once

#ifndef SUN_STATE_H_INCLUDED
#define SUN_STATE_H_INCLUDED

#include "scenes_manager/scenes_engine/helpers/clock_interface.h"
#include <math.h>
#include <stdint.h>
#include <chrono>
#include <stdexcept>
#include <functional>

class SunState {
public:
    struct Time {
        uint32_t hours;
        uint32_t minutes;
        uint32_t seconds;

        bool operator==( const Time& right ) const;
        int toSeconds() const;
    };
    class TimeZoneOffset {
    public:
        enum class Direction : int { BEFORE = +1, AFTER = -1 };
        using ClockGetter = std::function< SceneEngine::IClock&() >;

    public:
        TimeZoneOffset( uint8_t hours = 0, uint8_t minutes = 0, Direction direction = Direction::BEFORE );
        TimeZoneOffset( ClockGetter clockGetter );
        std::chrono::seconds getTimeZoneOffsetSeconds() const;
        std::chrono::minutes getTimeZoneOffsetMinutes() const;
        TimeZoneOffset::Direction getDirection() const;
        int getHours() const;
        int getMinutes() const;

    private:
        uint8_t m_hours{ 0 };
        uint8_t m_minutes{ 0 };
        Direction m_direction{ Direction::BEFORE };  //!< +1 means at and before Greenwich, -1 means after Greenwich

        ClockGetter m_clockGetter;  //!< Overrides previous fields, gets timezone information form SceneEngine clocks
    };

    constexpr static uint32_t NaN = 0x80000000;
    using ClockGetter = TimeZoneOffset::ClockGetter;

public:
    SunState();
    SunState( double lat, double lon, int timeZone );
    SunState( double lat, double lon, const ClockGetter& clockGetter );
    ~SunState();

    void setPosition( double lat, double lon, int timeZone );
    void setPosition( double lat, double lon, TimeZoneOffset timeZone );
    double setCurrentDate( int year, int month, int day );
    double calcSunriseUTC() const;
    double calcSunsetUTC() const;
    /* return time in minutes from midnight */
    double getSunriseMinutes() const;
    /* return time in minutes from midnight */
    double getSunsetMinutes() const;
    /* return time in seconds from midnight */
    double getSunriseSeconds() const;
    /* return time in seconds from midnight */
    double getSunsetSeconds() const;
    static int moonPhase( int fromepoch );
    int moonPhase();
    Time getSunriseTime();
    Time getSunsetTime();

private:
    static double degToRad( double );
    static double radToDeg( double );
    static double calcMeanObliquityOfEcliptic( double );
    static double calcGeomMeanLongSun( double );
    static double calcObliquityCorrection( double );
    static double calcEccentricityEarthOrbit( double );
    static double calcGeomMeanAnomalySun( double );
    static double calcEquationOfTime( double );
    static double calcTimeJulianCent( double );
    static double calcSunTrueLong( double );
    static double calcSunApparentLong( double );
    static double calcSunDeclination( double );
    static double calcHourAngleSunrise( double, double );
    static double calcHourAngleSunset( double, double );
    static double calcJD( int, int, int );
    static double calcJDFromJulianCent( double );
    static double calcSunEqOfCenter( double );
    static Time secondsToTime( double seconds );

private:
    double latitude{ 0.0 };
    double longitude{ 0.0 };
    double julianDate{ 0.0 };
    int m_year{ 0 };
    int m_month{ 0 };
    int m_day{ 0 };
    TimeZoneOffset tzOffset;
};

#endif
